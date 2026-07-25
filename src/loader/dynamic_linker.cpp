#include "loader/dynamic_linker.hpp"
#include "core/logging.hpp"
#include <cstring>

namespace quin::loader {

// ELF Relocation types for x86-64
constexpr uint32_t R_X86_64_NONE       = 0;
constexpr uint32_t R_X86_64_64         = 1;
constexpr uint32_t R_X86_64_GLOB_DAT   = 6;
constexpr uint32_t R_X86_64_JUMP_SLOT  = 7;
constexpr uint32_t R_X86_64_RELATIVE   = 8;

// Additional DT tags not in elf_types.hpp
constexpr int64_t DT_JMPREL  = 23;
constexpr int64_t DT_INIT    = 12;
constexpr int64_t DT_FINI    = 13;

DynamicLinker::DynamicLinker(quin::memory::GuestAddressSpace& memory,
                             quin::kernel::LibKernel& kernel,
                             quin::compat::CompatTriage& triage)
    : m_memory(memory), m_kernel(kernel), m_triage(triage) {}

bool DynamicLinker::link(const ParsedElf& elf, uint64_t load_base) {
    QUIN_LOG_INFO("DynamicLinker: Starting dynamic linking pass (load_base=0x{:016X})", load_base);

    if (!parse_dynamic_segment(elf, load_base)) {
        QUIN_LOG_WARN("DynamicLinker: No PT_DYNAMIC segment found — static binary, nothing to link.");
        return true; // Not an error for static binaries
    }

    // Log needed libraries
    for (const auto& lib : m_dyn_info.needed_libraries) {
        QUIN_LOG_INFO("DynamicLinker: DT_NEEDED — '{}'", lib);
    }

    resolve_relocations(load_base);

    QUIN_LOG_INFO("DynamicLinker: Linking complete — {} symbols resolved, {} unresolved.",
                  m_resolved.size(), m_unresolved.size());
    return true;
}

bool DynamicLinker::parse_dynamic_segment(const ParsedElf& elf, uint64_t load_base) {
    // Find the PT_DYNAMIC program header
    const Elf64_Phdr* dyn_phdr = nullptr;
    for (const auto& phdr : elf.program_headers) {
        if (phdr.p_type == PT_DYNAMIC) {
            dyn_phdr = &phdr;
            break;
        }
    }

    if (!dyn_phdr) {
        return false;
    }

    QUIN_LOG_INFO("DynamicLinker: Found PT_DYNAMIC at file offset 0x{:X}, vaddr 0x{:X}, size {}",
                  dyn_phdr->p_offset, dyn_phdr->p_vaddr, dyn_phdr->p_filesz);

    // Read dynamic entries from the loaded guest memory
    uint64_t dyn_vaddr = load_base + dyn_phdr->p_vaddr;
    size_t num_entries = dyn_phdr->p_filesz / sizeof(Elf64_Dyn);

    for (size_t i = 0; i < num_entries; ++i) {
        Elf64_Dyn dyn{};
        if (!m_memory.read_bytes(dyn_vaddr + i * sizeof(Elf64_Dyn), &dyn, sizeof(dyn))) {
            break;
        }

        if (dyn.d_tag == DT_NULL) break;

        switch (dyn.d_tag) {
            case DT_STRTAB:
                m_dyn_info.strtab_vaddr = load_base + dyn.d_un.d_ptr;
                break;
            case DT_SYMTAB:
                m_dyn_info.symtab_vaddr = load_base + dyn.d_un.d_ptr;
                break;
            case DT_RELA:
                m_dyn_info.rela_vaddr = load_base + dyn.d_un.d_ptr;
                break;
            case DT_RELASZ:
                m_dyn_info.rela_size = dyn.d_un.d_val;
                break;
            case DT_RELAENT:
                m_dyn_info.rela_entsize = dyn.d_un.d_val;
                break;
            case DT_JMPREL:
                m_dyn_info.jmprel_vaddr = load_base + dyn.d_un.d_ptr;
                break;
            case DT_PLTRELSZ:
                m_dyn_info.jmprel_size = dyn.d_un.d_val;
                break;
            case DT_INIT:
                m_dyn_info.init_vaddr = load_base + dyn.d_un.d_ptr;
                break;
            case DT_FINI:
                m_dyn_info.fini_vaddr = load_base + dyn.d_un.d_ptr;
                break;
            case DT_NEEDED: {
                // d_val is an offset into strtab — we'll resolve after we have strtab
                // Store as raw offset for now, resolve later
                m_dyn_info.needed_libraries.push_back(std::to_string(dyn.d_un.d_val));
                break;
            }
            default:
                break;
        }
    }

    // Now resolve DT_NEEDED library names from string table
    if (m_dyn_info.strtab_vaddr != 0) {
        std::vector<std::string> resolved_names;
        for (auto& name_or_offset : m_dyn_info.needed_libraries) {
            try {
                uint64_t offset = std::stoull(name_or_offset);
                std::string name = read_string_from_strtab(offset);
                if (!name.empty()) {
                    resolved_names.push_back(name);
                } else {
                    resolved_names.push_back(name_or_offset);
                }
            } catch (...) {
                resolved_names.push_back(name_or_offset);
            }
        }
        m_dyn_info.needed_libraries = std::move(resolved_names);
    }

    return true;
}

std::string DynamicLinker::read_string_from_strtab(uint64_t offset) {
    if (m_dyn_info.strtab_vaddr == 0) return "";

    std::string result;
    uint64_t addr = m_dyn_info.strtab_vaddr + offset;
    char ch = 0;
    for (size_t i = 0; i < 256; ++i) {
        if (!m_memory.read_bytes(addr + i, &ch, 1)) break;
        if (ch == '\0') break;
        result += ch;
    }
    return result;
}

void DynamicLinker::resolve_relocations(uint64_t load_base) {
    auto process_rela = [&](uint64_t rela_vaddr, uint64_t rela_total_size) {
        if (rela_vaddr == 0 || rela_total_size == 0) return;

        size_t entsize = (m_dyn_info.rela_entsize > 0) ? m_dyn_info.rela_entsize : sizeof(Elf64_Rela);
        size_t num_relas = rela_total_size / entsize;

        QUIN_LOG_INFO("DynamicLinker: Processing {} RELA entries at 0x{:016X}", num_relas, rela_vaddr);

        for (size_t i = 0; i < num_relas; ++i) {
            Elf64_Rela rela{};
            if (!m_memory.read_bytes(rela_vaddr + i * entsize, &rela, sizeof(rela))) {
                continue;
            }

            uint32_t sym_idx = ELF64_R_SYM(rela.r_info);
            uint32_t rel_type = ELF64_R_TYPE(rela.r_info);
            uint64_t target_vaddr = load_base + rela.r_offset;

            switch (rel_type) {
                case R_X86_64_RELATIVE: {
                    // RELATIVE: *target = load_base + addend
                    uint64_t value = load_base + static_cast<uint64_t>(rela.r_addend);
                    m_memory.write_bytes(target_vaddr, &value, 8);
                    break;
                }

                case R_X86_64_GLOB_DAT:
                case R_X86_64_JUMP_SLOT:
                case R_X86_64_64: {
                    // Look up symbol name from symtab + strtab
                    if (sym_idx == 0 || m_dyn_info.symtab_vaddr == 0) break;

                    Elf64_Sym sym{};
                    uint64_t sym_addr = m_dyn_info.symtab_vaddr + sym_idx * sizeof(Elf64_Sym);
                    if (!m_memory.read_bytes(sym_addr, &sym, sizeof(sym))) break;

                    std::string sym_name = read_string_from_strtab(sym.st_name);
                    if (sym_name.empty()) break;

                    ResolvedSymbol resolved_sym;
                    resolved_sym.name = sym_name;
                    resolved_sym.got_vaddr = target_vaddr;

                    if (m_kernel.has_symbol(sym_name)) {
                        // Symbol is available in our stub library
                        // Write a sentinel/trampoline address (we use a high-address range
                        // that the execution engine can trap on)
                        uint64_t trampoline = 0xFFFF'DEAD'0000'0000ULL + m_resolved.size();
                        m_memory.write_bytes(target_vaddr, &trampoline, 8);
                        resolved_sym.resolved_addr = trampoline;
                        resolved_sym.resolved = true;
                        m_resolved.push_back(resolved_sym);

                        QUIN_LOG_INFO("DynamicLinker: RESOLVED '{}' → GOT[0x{:016X}] = 0x{:016X}",
                                      sym_name, target_vaddr, trampoline);
                    } else {
                        // Unresolved — log to triage and write a trap address
                        uint64_t trap_addr = 0xFFFF'CAFE'0000'0000ULL + m_unresolved.size();
                        m_memory.write_bytes(target_vaddr, &trap_addr, 8);
                        m_unresolved.push_back(sym_name);
                        m_triage.log_missing_symbol(sym_name, "dynamic_linker");

                        QUIN_LOG_WARN("DynamicLinker: UNRESOLVED '{}' → GOT[0x{:016X}] = TRAP(0x{:016X})",
                                      sym_name, target_vaddr, trap_addr);
                    }
                    break;
                }

                case R_X86_64_NONE:
                default:
                    break;
            }
        }
    };

    // Process DT_RELA entries
    process_rela(m_dyn_info.rela_vaddr, m_dyn_info.rela_size);

    // Process DT_JMPREL entries (PLT relocations)
    process_rela(m_dyn_info.jmprel_vaddr, m_dyn_info.jmprel_size);
}

} // namespace quin::loader
