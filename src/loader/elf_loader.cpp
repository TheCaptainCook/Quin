#include "loader/elf_loader.hpp"
#include "core/logging.hpp"
#include <cstring>

namespace quin::loader {

ElfLoader::ElfLoader(quin::memory::GuestAddressSpace& address_space)
    : m_memory(address_space) {}

LoadResult ElfLoader::load(const ParsedElf& parsed_elf) {
    LoadResult result{};
    if (!parsed_elf.valid) {
        QUIN_LOG_ERROR("ElfLoader: Invalid ELF structure provided.");
        return result;
    }

    QUIN_LOG_INFO("ElfLoader: Commencing segment mapping for entry point 0x{:016X}...", parsed_elf.entry_point);

    for (const auto& ph : parsed_elf.program_headers) {
        if (ph.p_type != PT_LOAD) continue;

        // Determine permissions
        quin::memory::PagePermission perm = quin::memory::PagePermission::None;
        if (ph.p_flags & PF_R) perm = static_cast<quin::memory::PagePermission>(static_cast<uint32_t>(perm) | static_cast<uint32_t>(quin::memory::PagePermission::Read));
        if (ph.p_flags & PF_W) perm = static_cast<quin::memory::PagePermission>(static_cast<uint32_t>(perm) | static_cast<uint32_t>(quin::memory::PagePermission::Write));
        if (ph.p_flags & PF_X) perm = static_cast<quin::memory::PagePermission>(static_cast<uint32_t>(perm) | static_cast<uint32_t>(quin::memory::PagePermission::Execute));

        uint64_t vaddr = ph.p_vaddr;
        size_t mem_size = static_cast<size_t>(ph.p_memsz);
        size_t file_size = static_cast<size_t>(ph.p_filesz);
        size_t file_offset = static_cast<size_t>(ph.p_offset);

        if (mem_size == 0) continue;

        // Allocate segment in guest address space
        if (!m_memory.allocate(vaddr, mem_size, perm)) {
            QUIN_LOG_ERROR("ElfLoader: Failed to allocate segment at VAddr 0x{:016X}", vaddr);
            return {};
        }

        // Copy binary file payload into mapped memory
        if (file_size > 0 && file_offset + file_size <= parsed_elf.raw_data.size()) {
            if (!m_memory.write_bytes(vaddr, parsed_elf.raw_data.data() + file_offset, file_size)) {
                QUIN_LOG_ERROR("ElfLoader: Failed to copy segment payload to VAddr 0x{:016X}", vaddr);
                return {};
            }
        }

        result.loaded_segments_count++;
        result.total_mapped_bytes += mem_size;
    }

    // Set up default Guest Stack (2 MB at 0x00007FFFF0000000)
    uint64_t stack_base = 0x00007FFFF0000000ULL;
    size_t stack_size = 2 * 1024 * 1024; // 2 MB
    if (m_memory.allocate(stack_base, stack_size, quin::memory::PagePermission::ReadWrite)) {
        result.stack_top = stack_base + stack_size - 64; // Alignment offset
        QUIN_LOG_INFO("ElfLoader: Guest Stack allocated at VAddr 0x{:016X} (Top: 0x{:016X})", stack_base, result.stack_top);
    }

    result.entry_point = parsed_elf.entry_point;
    result.success = true;

    QUIN_LOG_INFO("ElfLoader: Successfully loaded {} segments ({} total bytes mapped). Ready for bootstrap.",
                  result.loaded_segments_count, result.total_mapped_bytes);
    return result;
}

} // namespace quin::loader
