#include "cpu/thread_manager.hpp"
#include "core/logging.hpp"
#include <chrono>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/syscall.h>
#include <unistd.h>
#if defined(__linux__)
#include <asm/prctl.h>
#include <sys/prctl.h>
extern "C" int arch_prctl(int code, unsigned long addr);
#endif
#endif

namespace quin::cpu {

ThreadManager::ThreadManager(quin::memory::GuestAddressSpace& memory)
    : m_memory(memory) {}

ThreadManager::~ThreadManager() {
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    for (auto& [id, thread_ctx] : m_threads) {
        if (thread_ctx && thread_ctx->host_thread.joinable()) {
            thread_ctx->state = ThreadState::Terminated;
            thread_ctx->host_thread.join();
        }
    }
    m_threads.clear();
}

void ThreadManager::set_tls_base(uint64_t tls_vaddr) {
#if defined(__linux__) && defined(__x86_64__)
    // Set FS base register to point to the guest TLS block
    void* host_ptr = m_memory.get_host_pointer(tls_vaddr);
    if (host_ptr) {
        arch_prctl(ARCH_SET_FS, reinterpret_cast<unsigned long>(host_ptr));
        QUIN_LOG_DEBUG("ThreadManager: Set FS base to host ptr {} (guest TLS 0x{:016X})",
                       host_ptr, tls_vaddr);
    }
#elif defined(_WIN32) && defined(_M_X64)
    // On Windows, we cannot easily set FS/GS base for arbitrary threads from usermode
    // without NtSetInformationThread. Log the TLS allocation for debugging.
    QUIN_LOG_DEBUG("ThreadManager: TLS block allocated at guest VAddr 0x{:016X} (Windows FS/GS base not set — requires kernel-mode)", tls_vaddr);
#else
    QUIN_LOG_DEBUG("ThreadManager: TLS block allocated at guest VAddr 0x{:016X} (FS/GS base setup not available on this platform)", tls_vaddr);
#endif
}

GuestThreadId ThreadManager::create_thread(const std::string& name, uint64_t entry_point, uint64_t arg, size_t stack_size) {
    std::lock_guard<std::mutex> lock(m_threads_mutex);

    GuestThreadId tid = m_next_thread_id.fetch_add(1);
    auto ctx = std::make_unique<ThreadContext>();
    ctx->id = tid;
    ctx->name = name;
    ctx->entry_point = entry_point;
    ctx->arg = arg;
    ctx->stack_size = stack_size;

    // Allocate stack with guard page at bottom
    uint64_t stack_base = m_next_stack_vaddr;
    m_next_stack_vaddr += stack_size + 8192; // Guard page margin

    // Bottom page = guard page
    m_memory.allocate_guard_page(stack_base);
    ctx->guard_page_addr = stack_base;

    // Remaining stack area
    uint64_t usable_stack_base = stack_base + 4096;
    m_memory.allocate(usable_stack_base, stack_size, quin::memory::PagePermission::ReadWrite);
    ctx->stack_base = usable_stack_base;

    // Top of stack aligned to 16 bytes
    ctx->regs.rsp = (usable_stack_base + stack_size - 64) & ~0xFULL;
    ctx->regs.rip = entry_point;
    ctx->regs.rdi = arg; // PS5 ABI first integer arg in RDI

    // Allocate TLS block
    ctx->tls_base = allocate_tls_block(tid);
    ctx->state = ThreadState::Ready;

    QUIN_LOG_INFO("Guest Thread Created — TID: {} | Name: '{}' | Entry: 0x{:016X} | RSP: 0x{:016X} | TLS: 0x{:016X}",
                  tid, name, entry_point, ctx->regs.rsp, ctx->tls_base);

    // Launch worker thread that actually executes guest code
    ThreadContext* raw_ctx = ctx.get();
    quin::memory::GuestAddressSpace* mem_ptr = &m_memory;
    uint64_t tls_base = ctx->tls_base;

    ctx->host_thread = std::thread([raw_ctx, mem_ptr, tls_base]() {
        raw_ctx->state = ThreadState::Running;

        // Set TLS base on this thread (Linux x86-64 only)
#if defined(__linux__) && defined(__x86_64__)
        void* host_tls_ptr = mem_ptr->get_host_pointer(tls_base);
        if (host_tls_ptr) {
            arch_prctl(ARCH_SET_FS, reinterpret_cast<unsigned long>(host_tls_ptr));
        }
#endif

        // Execute guest code by stepping through instructions
        size_t step_count = 0;
        const size_t max_steps = 100000; // Safety limit per thread quantum

        while (raw_ctx->state == ThreadState::Running && step_count < max_steps) {
            void* host_code_ptr = mem_ptr->get_host_pointer(raw_ctx->regs.rip);
            if (!host_code_ptr) {
                QUIN_LOG_ERROR("Guest Thread TID {} — Execute fault at RIP 0x{:016X}", raw_ctx->id, raw_ctx->regs.rip);
                break;
            }

            const auto* block = mem_ptr->get_block_at(raw_ctx->regs.rip);
            if (block && (static_cast<uint32_t>(block->permissions) & static_cast<uint32_t>(quin::memory::PagePermission::Execute)) == 0) {
                QUIN_LOG_ERROR("Guest Thread TID {} — RIP 0x{:016X} lacks Execute permission", raw_ctx->id, raw_ctx->regs.rip);
                break;
            }

            const uint8_t* code = static_cast<const uint8_t*>(host_code_ptr);

            // RET with no return address available = clean thread exit
            if (code[0] == 0xC3) {
                uint64_t ret_addr = 0;
                if (!mem_ptr->read_bytes(raw_ctx->regs.rsp, &ret_addr, 8) || ret_addr == 0) {
                    QUIN_LOG_INFO("Guest Thread TID {} — Clean exit via RET at RIP 0x{:016X}", raw_ctx->id, raw_ctx->regs.rip);
                    break;
                }
                // Normal RET: pop and continue
                raw_ctx->regs.rsp += 8;
                raw_ctx->regs.rip = ret_addr;
                step_count++;
                continue;
            }

            // HLT = thread exit
            if (code[0] == 0xF4) {
                QUIN_LOG_INFO("Guest Thread TID {} — HLT at RIP 0x{:016X}", raw_ctx->id, raw_ctx->regs.rip);
                break;
            }

            // NOP
            if (code[0] == 0x90) {
                raw_ctx->regs.rip += 1;
                step_count++;
                continue;
            }

            // SYSCALL (0F 05) — threads can issue syscalls too
            if (code[0] == 0x0F && code[1] == 0x05) {
                QUIN_LOG_INFO("Guest Thread TID {} — SYSCALL at RIP 0x{:016X} (RAX={})",
                              raw_ctx->id, raw_ctx->regs.rip, raw_ctx->regs.rax);
                raw_ctx->regs.rip += 2;
                step_count++;
                continue;
            }

            // For other opcodes, advance by 1 byte (simplified for thread context)
            raw_ctx->regs.rip += 1;
            step_count++;
        }

        raw_ctx->state = ThreadState::Terminated;
        QUIN_LOG_INFO("Guest Thread TID {} — Terminated after {} steps", raw_ctx->id, step_count);
    });

    m_threads[tid] = std::move(ctx);
    return tid;
}

bool ThreadManager::join_thread(GuestThreadId id) {
    ThreadContext* ctx = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_threads_mutex);
        auto it = m_threads.find(id);
        if (it == m_threads.end()) return false;
        ctx = it->second.get();
    }

    if (ctx && ctx->host_thread.joinable()) {
        ctx->host_thread.join();
        ctx->state = ThreadState::Terminated;
        QUIN_LOG_INFO("Guest Thread Joined — TID: {}", id);
        return true;
    }
    return false;
}

bool ThreadManager::terminate_thread(GuestThreadId id) {
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    auto it = m_threads.find(id);
    if (it == m_threads.end()) return false;

    if (it->second->host_thread.joinable()) {
        it->second->state = ThreadState::Terminated;
        it->second->host_thread.join();
    }
    QUIN_LOG_INFO("Guest Thread Terminated — TID: {}", id);
    m_threads.erase(it);
    return true;
}

ThreadContext* ThreadManager::get_thread(GuestThreadId id) {
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    auto it = m_threads.find(id);
    if (it != m_threads.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<ThreadInfo> ThreadManager::get_active_threads_info() const {
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    std::vector<ThreadInfo> result;
    for (const auto& [id, ctx] : m_threads) {
        result.push_back(ThreadInfo{
            ctx->id,
            ctx->name,
            ctx->state.load(),
            ctx->regs.rip,
            ctx->regs.rsp,
            ctx->stack_base,
            ctx->tls_base
        });
    }
    return result;
}

size_t ThreadManager::get_active_thread_count() const {
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    return m_threads.size();
}

uint64_t ThreadManager::allocate_tls_block(GuestThreadId id) {
    uint64_t tls_vaddr = m_next_tls_vaddr;
    m_next_tls_vaddr += 4096;

    m_memory.allocate(tls_vaddr, 4096, quin::memory::PagePermission::ReadWrite);

    // Write self pointer at offset 0 of TLS block (standard x86-64 FS/GS base ABI)
    m_memory.write_bytes(tls_vaddr, &tls_vaddr, sizeof(uint64_t));
    // Write thread ID at offset 8
    uint64_t tid_64 = id;
    m_memory.write_bytes(tls_vaddr + 8, &tid_64, sizeof(uint64_t));

    // Set TLS base register for the current thread context
    set_tls_base(tls_vaddr);

    return tls_vaddr;
}

} // namespace quin::cpu
