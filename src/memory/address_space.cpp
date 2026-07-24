#include "memory/address_space.hpp"
#include "core/logging.hpp"
#include <cstring>
#include <algorithm>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace quin::memory {

namespace {

#if defined(_WIN32)
DWORD to_win32_prot(PagePermission perm) {
    if (perm == PagePermission::None) return PAGE_NOACCESS;
    if (perm == PagePermission::Read) return PAGE_READONLY;
    if (perm == PagePermission::ReadWrite) return PAGE_READWRITE;
    if (perm == PagePermission::Execute) return PAGE_EXECUTE;
    if (perm == PagePermission::ReadExecute) return PAGE_EXECUTE_READWRITE;
    return PAGE_EXECUTE_READWRITE;
}
#else
int to_posix_prot(PagePermission perm) {
    if (perm == PagePermission::None) return PROT_NONE;
    int prot = 0;
    if (static_cast<uint32_t>(perm) & static_cast<uint32_t>(PagePermission::Read)) prot |= PROT_READ;
    if (static_cast<uint32_t>(perm) & static_cast<uint32_t>(PagePermission::Write)) prot |= PROT_WRITE;
    if (static_cast<uint32_t>(perm) & static_cast<uint32_t>(PagePermission::Execute)) prot |= PROT_EXEC;
    return prot;
}
#endif

} // anonymous namespace

GuestAddressSpace::GuestAddressSpace() = default;

GuestAddressSpace::~GuestAddressSpace() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& block : m_blocks) {
        if (block.host_ptr) {
#if defined(_WIN32)
            VirtualFree(block.host_ptr, 0, MEM_RELEASE);
#else
            munmap(block.host_ptr, block.size);
#endif
        }
    }
    m_blocks.clear();
}

bool GuestAddressSpace::allocate(uint64_t guest_vaddr, size_t size, PagePermission permissions) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (size == 0) return false;

    size_t page_size = 4096;
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);

    void* host_memory = nullptr;
#if defined(_WIN32)
    DWORD win_prot = (permissions == PagePermission::None) ? PAGE_NOACCESS : PAGE_EXECUTE_READWRITE;
    host_memory = VirtualAlloc(nullptr, aligned_size, MEM_COMMIT | MEM_RESERVE, win_prot);
#else
    int prot = (permissions == PagePermission::None) ? PROT_NONE : (PROT_READ | PROT_WRITE | PROT_EXEC);
    host_memory = mmap(nullptr, aligned_size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (host_memory == MAP_FAILED) host_memory = nullptr;
#endif

    if (!host_memory) {
        QUIN_LOG_ERROR("Failed to allocate {} bytes of virtual memory for guest VAddr 0x{:016X}", aligned_size, guest_vaddr);
        return false;
    }

    if (permissions != PagePermission::None) {
        std::memset(host_memory, 0, aligned_size);
    }

    MemoryBlock block{
        guest_vaddr,
        aligned_size,
        permissions,
        host_memory
    };

    m_blocks.push_back(block);
    m_total_allocated += aligned_size;

    QUIN_LOG_INFO("Guest Memory Allocated — VAddr: 0x{:016X} | Size: 0x{:X} bytes | HostPtr: {}",
                  guest_vaddr, aligned_size, host_memory);
    return true;
}

bool GuestAddressSpace::free(uint64_t guest_vaddr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_blocks.begin(), m_blocks.end(), [guest_vaddr](const MemoryBlock& b) {
        return b.guest_vaddr == guest_vaddr;
    });

    if (it != m_blocks.end()) {
        if (it->host_ptr) {
#if defined(_WIN32)
            VirtualFree(it->host_ptr, 0, MEM_RELEASE);
#else
            munmap(it->host_ptr, it->size);
#endif
        }
        m_total_allocated -= it->size;
        m_blocks.erase(it);
        QUIN_LOG_INFO("Guest Memory Freed — VAddr: 0x{:016X}", guest_vaddr);
        return true;
    }
    return false;
}

bool GuestAddressSpace::mprotect(uint64_t guest_vaddr, size_t size, PagePermission permissions) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& block : m_blocks) {
        if (guest_vaddr >= block.guest_vaddr && guest_vaddr + size <= block.guest_vaddr + block.size) {
            uint64_t offset = guest_vaddr - block.guest_vaddr;
            void* host_ptr = static_cast<uint8_t*>(block.host_ptr) + offset;

#if defined(_WIN32)
            DWORD old_prot = 0;
            DWORD new_prot = to_win32_prot(permissions);
            if (!VirtualProtect(host_ptr, size, new_prot, &old_prot)) {
                QUIN_LOG_ERROR("VirtualProtect failed at VAddr 0x{:016X} (Error: {})", guest_vaddr, GetLastError());
                return false;
            }
#else
            int new_prot = to_posix_prot(permissions);
            if (::mprotect(host_ptr, size, new_prot) != 0) {
                QUIN_LOG_ERROR("mprotect failed at VAddr 0x{:016X}", guest_vaddr);
                return false;
            }
#endif
            block.permissions = permissions;
            QUIN_LOG_INFO("Guest Memory Protected — VAddr: 0x{:016X} | Size: 0x{:X} | NewPerms: {}",
                          guest_vaddr, size, static_cast<uint32_t>(permissions));
            return true;
        }
    }
    QUIN_LOG_ERROR("mprotect failed: VAddr 0x{:016X} not found in mapped memory", guest_vaddr);
    return false;
}

uint64_t GuestAddressSpace::mmap(uint64_t guest_vaddr, size_t size, PagePermission permissions, uint32_t /*flags*/) {
    uint64_t target_vaddr = guest_vaddr;
    if (target_vaddr == 0) {
        target_vaddr = m_next_auto_vaddr;
        m_next_auto_vaddr += (size + 4095) & ~4095;
    }

    if (allocate(target_vaddr, size, permissions)) {
        return target_vaddr;
    }
    return 0;
}

bool GuestAddressSpace::munmap(uint64_t guest_vaddr, size_t /*size*/) {
    return free(guest_vaddr);
}

bool GuestAddressSpace::allocate_guard_page(uint64_t guest_vaddr) {
    return allocate(guest_vaddr, 4096, PagePermission::None);
}

void* GuestAddressSpace::get_host_pointer(uint64_t guest_vaddr) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& block : m_blocks) {
        if (guest_vaddr >= block.guest_vaddr && guest_vaddr < block.guest_vaddr + block.size) {
            uint64_t offset = guest_vaddr - block.guest_vaddr;
            return static_cast<uint8_t*>(block.host_ptr) + offset;
        }
    }
    return nullptr;
}

const MemoryBlock* GuestAddressSpace::get_block_at(uint64_t guest_vaddr) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& block : m_blocks) {
        if (guest_vaddr >= block.guest_vaddr && guest_vaddr < block.guest_vaddr + block.size) {
            return &block;
        }
    }
    return nullptr;
}

bool GuestAddressSpace::write_bytes(uint64_t guest_vaddr, const void* src, size_t size) {
    void* host_ptr = get_host_pointer(guest_vaddr);
    if (!host_ptr) {
        QUIN_LOG_ERROR("Guest memory write fault at unmapped address 0x{:016X}", guest_vaddr);
        return false;
    }
    std::memcpy(host_ptr, src, size);
    return true;
}

bool GuestAddressSpace::read_bytes(uint64_t guest_vaddr, void* dest, size_t size) const {
    void* host_ptr = get_host_pointer(guest_vaddr);
    if (!host_ptr) {
        QUIN_LOG_ERROR("Guest memory read fault at unmapped address 0x{:016X}", guest_vaddr);
        return false;
    }
    std::memcpy(dest, host_ptr, size);
    return true;
}

size_t GuestAddressSpace::get_total_allocated_bytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_total_allocated;
}

std::vector<MemoryBlock> GuestAddressSpace::get_blocks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blocks;
}

} // namespace quin::memory
