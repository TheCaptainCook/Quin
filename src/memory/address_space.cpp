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

GuestAddressSpace::GuestAddressSpace() = default;

GuestAddressSpace::~GuestAddressSpace() {
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
    if (size == 0) return false;

    // Page align size (4096 bytes)
    size_t page_size = 4096;
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);

    void* host_memory = nullptr;
#if defined(_WIN32)
    DWORD win_prot = PAGE_READWRITE;
    if (permissions == PagePermission::Execute || permissions == PagePermission::ReadExecute || permissions == PagePermission::All) {
        win_prot = PAGE_EXECUTE_READWRITE;
    }
    host_memory = VirtualAlloc(nullptr, aligned_size, MEM_COMMIT | MEM_RESERVE, win_prot);
#else
    int prot = PROT_READ | PROT_WRITE;
    if (permissions == PagePermission::Execute || permissions == PagePermission::ReadExecute || permissions == PagePermission::All) {
        prot |= PROT_EXEC;
    }
    host_memory = mmap(nullptr, aligned_size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (host_memory == MAP_FAILED) host_memory = nullptr;
#endif

    if (!host_memory) {
        QUIN_LOG_ERROR("Failed to allocate {} bytes of virtual memory for guest VAddr 0x{:016X}", aligned_size, guest_vaddr);
        return false;
    }

    std::memset(host_memory, 0, aligned_size);

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

void* GuestAddressSpace::get_host_pointer(uint64_t guest_vaddr) const {
    for (const auto& block : m_blocks) {
        if (guest_vaddr >= block.guest_vaddr && guest_vaddr < block.guest_vaddr + block.size) {
            uint64_t offset = guest_vaddr - block.guest_vaddr;
            return static_cast<uint8_t*>(block.host_ptr) + offset;
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

} // namespace quin::memory
