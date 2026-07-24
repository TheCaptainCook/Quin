#ifndef QUIN_LOADER_SELF_PARSER_HPP
#define QUIN_LOADER_SELF_PARSER_HPP

#include "loader/elf_types.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace quin::loader {

struct ParsedElf {
    bool valid{false};
    bool is_self{false};
    Elf64_Ehdr header{};
    std::vector<Elf64_Phdr> program_headers;
    std::vector<Elf64_Shdr> section_headers;
    std::vector<std::string> needed_libraries;
    std::vector<std::string> imported_symbols;
    std::vector<uint8_t> raw_data;
    uint64_t entry_point{0};
};

class SelfParser {
public:
    SelfParser() = default;

    static ParsedElf parse_buffer(const std::vector<uint8_t>& buffer);
    static ParsedElf parse_file(const std::string& filepath);
};

} // namespace quin::loader

#endif // QUIN_LOADER_SELF_PARSER_HPP
