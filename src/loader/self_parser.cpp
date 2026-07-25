#include "loader/self_parser.hpp"
#include "core/logging.hpp"
#include <fstream>
#include <cstring>

namespace quin::loader {

ParsedElf SelfParser::parse_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        QUIN_LOG_ERROR("Failed to open binary file: {}", filepath);
        return {};
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        QUIN_LOG_ERROR("Failed to read file buffer for: {}", filepath);
        return {};
    }

    return parse_buffer(buffer);
}

ParsedElf SelfParser::parse_buffer(const std::vector<uint8_t>& buffer) {
    ParsedElf result{};
    result.raw_data = buffer;

    if (buffer.size() < sizeof(Elf64_Ehdr)) {
        QUIN_LOG_ERROR("Buffer size ({} bytes) too small for ELF header", buffer.size());
        return result;
    }

    size_t elf_offset = 0;

    // Check SELF magic (0x4F534F4E "NOSO" / 0x4F4F5345 "ESOO") or ELF magic
    if (buffer[0] == ELF_MAG0 && buffer[1] == ELF_MAG1 && buffer[2] == ELF_MAG2 && buffer[3] == ELF_MAG3) {
        result.is_self = false;
        elf_offset = 0;
        QUIN_LOG_INFO("Valid standard 64-bit ELF binary header detected.");
    } else if (buffer.size() >= 32 && buffer[0] == 'O' && buffer[1] == 'S' && buffer[2] == 'E') {
        result.is_self = true;
        // SELF header wraps ELF at standard offset (e.g. 0x20 or 0x40 depending on header version)
        elf_offset = 0x20;
        QUIN_LOG_INFO("SELF (Signed ELF) container header detected. Unwrapping inner ELF header...");
    } else {
        QUIN_LOG_ERROR("Invalid ELF/SELF magic header bytes: 0x{:02X} 0x{:02X} 0x{:02X} 0x{:02X}",
                       buffer[0], buffer[1], buffer[2], buffer[3]);
        return result;
    }

    if (buffer.size() < elf_offset + sizeof(Elf64_Ehdr)) {
        QUIN_LOG_ERROR("Buffer size insufficient for inner ELF header");
        return result;
    }

    std::memcpy(&result.header, buffer.data() + elf_offset, sizeof(Elf64_Ehdr));

    // Validate 64-bit Little-Endian Architecture
    if (result.header.e_ident[4] != ELFCLASS64) {
        QUIN_LOG_ERROR("Unsupported ELF class (expected 64-bit)");
        return result;
    }

    if (result.header.e_ident[5] != ELFDATA2LSB) {
        QUIN_LOG_ERROR("Unsupported endianness (expected Little-Endian)");
        return result;
    }

    if (result.header.e_machine != EM_X86_64) {
        QUIN_LOG_WARN("Non x86-64 machine ID: {} (expected x86-64: 62)", result.header.e_machine);
    }

    result.entry_point = result.header.e_entry;
    QUIN_LOG_INFO("ELF Header Parsed — Entry Point: 0x{:016X}, Program Headers Count: {}",
                  result.entry_point, result.header.e_phnum);

    // Read Program Headers
    if (result.header.e_phnum > 0) {
        size_t ph_offset = elf_offset + result.header.e_phoff;
        if (buffer.size() >= ph_offset + (result.header.e_phnum * sizeof(Elf64_Phdr))) {
            result.program_headers.resize(result.header.e_phnum);
            std::memcpy(result.program_headers.data(),
                        buffer.data() + ph_offset,
                        result.header.e_phnum * sizeof(Elf64_Phdr));

            for (size_t i = 0; i < result.program_headers.size(); ++i) {
                const auto& ph = result.program_headers[i];
                if (ph.p_type == PT_LOAD) {
                    QUIN_LOG_INFO("  Segment [{}]: PT_LOAD | VAddr: 0x{:016X} | FileSz: 0x{:X} | MemSz: 0x{:X} | Flags: {}{}{}",
                                  i, ph.p_vaddr, ph.p_filesz, ph.p_memsz,
                                  (ph.p_flags & PF_R) ? "R" : "-",
                                  (ph.p_flags & PF_W) ? "W" : "-",
                                  (ph.p_flags & PF_X) ? "X" : "-");
                }
            }
        } else {
            QUIN_LOG_ERROR("Buffer truncated when reading program headers.");
            return result;
        }
    }

    // =========================================================================
    // Parse PT_DYNAMIC segment to extract DT_NEEDED libraries and symbols
    // =========================================================================
    for (const auto& ph : result.program_headers) {
        if (ph.p_type == PT_DYNAMIC) {
            QUIN_LOG_INFO("  Found PT_DYNAMIC segment at file offset 0x{:X}, size {}", ph.p_offset, ph.p_filesz);

            size_t dyn_file_offset = elf_offset + ph.p_offset;
            if (dyn_file_offset + ph.p_filesz > buffer.size()) {
                QUIN_LOG_WARN("  PT_DYNAMIC segment extends beyond buffer — skipping.");
                break;
            }

            // First pass: find DT_STRTAB offset and size
            uint64_t strtab_offset = 0;
            size_t num_dyn_entries = ph.p_filesz / sizeof(Elf64_Dyn);
            std::vector<uint64_t> needed_offsets;

            for (size_t i = 0; i < num_dyn_entries; ++i) {
                Elf64_Dyn dyn{};
                size_t entry_pos = dyn_file_offset + i * sizeof(Elf64_Dyn);
                if (entry_pos + sizeof(Elf64_Dyn) > buffer.size()) break;
                std::memcpy(&dyn, buffer.data() + entry_pos, sizeof(Elf64_Dyn));

                if (dyn.d_tag == DT_NULL) break;

                if (dyn.d_tag == DT_STRTAB) {
                    strtab_offset = dyn.d_un.d_ptr;
                } else if (dyn.d_tag == DT_NEEDED) {
                    needed_offsets.push_back(dyn.d_un.d_val);
                } else if (dyn.d_tag == DT_SYMTAB) {
                    QUIN_LOG_INFO("  DT_SYMTAB at VAddr: 0x{:016X}", dyn.d_un.d_ptr);
                }
            }

            // Resolve DT_NEEDED names from string table
            if (strtab_offset > 0) {
                // Try to find strtab in file: look for a PT_LOAD segment covering strtab_offset
                for (const auto& load_ph : result.program_headers) {
                    if (load_ph.p_type == PT_LOAD &&
                        strtab_offset >= load_ph.p_vaddr &&
                        strtab_offset < load_ph.p_vaddr + load_ph.p_filesz) {
                        
                        uint64_t strtab_file_base = elf_offset + load_ph.p_offset + (strtab_offset - load_ph.p_vaddr);
                        
                        for (uint64_t name_off : needed_offsets) {
                            uint64_t str_pos = strtab_file_base + name_off;
                            if (str_pos < buffer.size()) {
                                std::string lib_name;
                                for (size_t c = 0; str_pos + c < buffer.size() && buffer[str_pos + c] != 0 && c < 256; ++c) {
                                    lib_name += static_cast<char>(buffer[str_pos + c]);
                                }
                                if (!lib_name.empty()) {
                                    result.needed_libraries.push_back(lib_name);
                                    QUIN_LOG_INFO("  DT_NEEDED: '{}'", lib_name);
                                }
                            }
                        }
                        break;
                    }
                }
            }
            break; // Only one PT_DYNAMIC segment
        }
    }

    result.valid = true;
    return result;
}

} // namespace quin::loader
