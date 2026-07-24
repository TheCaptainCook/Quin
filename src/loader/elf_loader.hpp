#ifndef QUIN_LOADER_ELF_LOADER_HPP
#define QUIN_LOADER_ELF_LOADER_HPP

#include "loader/self_parser.hpp"
#include "memory/address_space.hpp"
#include <cstdint>
#include <string>

namespace quin::loader {

struct LoadResult {
    bool success{false};
    uint64_t entry_point{0};
    uint64_t stack_top{0};
    size_t loaded_segments_count{0};
    size_t total_mapped_bytes{0};
};

class ElfLoader {
public:
    explicit ElfLoader(quin::memory::GuestAddressSpace& address_space);

    LoadResult load(const ParsedElf& parsed_elf);

private:
    quin::memory::GuestAddressSpace& m_memory;
};

} // namespace quin::loader

#endif // QUIN_LOADER_ELF_LOADER_HPP
