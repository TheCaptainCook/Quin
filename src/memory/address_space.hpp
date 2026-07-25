#ifndef QUIN_MEMORY_ADDRESS_SPACE_HPP
#define QUIN_MEMORY_ADDRESS_SPACE_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>
#include <mutex>

namespace quin::memory {

enum class PagePermission : uint32_t {
    None        = 0,
    Read        = 1 << 0,
    Write       = 1 << 1,
    Execute     = 1 << 2,
    ReadWrite   = Read | Write,
    ReadExecute = Read | Execute,
    ReadWriteExecute = Read | Write | Execute,
    All         = Read | Write | Execute
};

struct MemoryBlock {
    uint64_t guest_vaddr{0};
    size_t size{0};
    PagePermission permissions{PagePermission::ReadWrite};
    void* host_ptr{nullptr};
};

class GuestAddressSpace {
public:
    GuestAddressSpace();
    ~GuestAddressSpace();

    // Prevent copying
    GuestAddressSpace(const GuestAddressSpace&) = delete;
    GuestAddressSpace& operator=(const GuestAddressSpace&) = delete;

    bool allocate(uint64_t guest_vaddr, size_t size, PagePermission permissions);
    bool free(uint64_t guest_vaddr);

    // Phase 2 Memory Enhancements
    bool mprotect(uint64_t guest_vaddr, size_t size, PagePermission permissions);
    uint64_t mmap(uint64_t guest_vaddr, size_t size, PagePermission permissions, uint32_t flags = 0);
    bool munmap(uint64_t guest_vaddr, size_t size);
    bool allocate_guard_page(uint64_t guest_vaddr);

    bool write_bytes(uint64_t guest_vaddr, const void* src, size_t size);
    bool read_bytes(uint64_t guest_vaddr, void* dest, size_t size) const;

    void* get_host_pointer(uint64_t guest_vaddr) const;
    const MemoryBlock* get_block_at(uint64_t guest_vaddr) const;

    size_t get_total_allocated_bytes() const;
    std::vector<MemoryBlock> get_blocks() const;

private:
    std::vector<MemoryBlock> m_blocks;
    size_t m_total_allocated{0};
    uint64_t m_next_auto_vaddr{0x0000000100000000ULL};
    mutable std::mutex m_mutex;
};

} // namespace quin::memory

#endif // QUIN_MEMORY_ADDRESS_SPACE_HPP
