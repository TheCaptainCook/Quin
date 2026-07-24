#include "cpu/thread_manager.hpp"
#include "core/logging.hpp"
#include <chrono>

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

    // Launch worker thread
    ThreadContext* raw_ctx = ctx.get();
    ctx->host_thread = std::thread([raw_ctx]() {
        raw_ctx->state = ThreadState::Running;
        // Basic thread harness placeholder
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        raw_ctx->state = ThreadState::Terminated;
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

    return tls_vaddr;
}

} // namespace quin::cpu
