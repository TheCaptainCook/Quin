#ifndef QUIN_LOADER_DYNAMIC_LINKER_HPP
#define QUIN_LOADER_DYNAMIC_LINKER_HPP

#include "loader/self_parser.hpp"
#include "loader/elf_types.hpp"
#include "memory/address_space.hpp"
#include "kernel/libkernel.hpp"
#include "compat/compat_triage.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace quin::loader {

struct ResolvedSymbol {
    std::string name;
    uint64_t got_vaddr{0};      // Address in GOT where the resolved address should be written
    uint64_t resolved_addr{0};  // The resolved address (trampoline or stub)
    bool resolved{false};
};

struct DynamicInfo {
    uint64_t strtab_vaddr{0};
    uint64_t symtab_vaddr{0};
    uint64_t rela_vaddr{0};
    uint64_t rela_size{0};
    uint64_t rela_entsize{0};
    uint64_t jmprel_vaddr{0};
    uint64_t jmprel_size{0};
    uint64_t init_vaddr{0};
    uint64_t fini_vaddr{0};
    std::vector<std::string> needed_libraries;
};

class DynamicLinker {
public:
    DynamicLinker(quin::memory::GuestAddressSpace& memory,
                  quin::kernel::LibKernel& kernel,
                  quin::compat::CompatTriage& triage);

    // Parse PT_DYNAMIC from a loaded ELF and resolve symbols
    bool link(const ParsedElf& elf, uint64_t load_base);

    const std::vector<ResolvedSymbol>& get_resolved_symbols() const { return m_resolved; }
    const std::vector<std::string>& get_unresolved_symbols() const { return m_unresolved; }
    const DynamicInfo& get_dynamic_info() const { return m_dyn_info; }

    size_t get_total_resolved() const { return m_resolved.size(); }
    size_t get_total_unresolved() const { return m_unresolved.size(); }

private:
    bool parse_dynamic_segment(const ParsedElf& elf, uint64_t load_base);
    void resolve_relocations(uint64_t load_base);
    std::string read_string_from_strtab(uint64_t offset);

    quin::memory::GuestAddressSpace& m_memory;
    quin::kernel::LibKernel& m_kernel;
    quin::compat::CompatTriage& m_triage;

    DynamicInfo m_dyn_info{};
    std::vector<ResolvedSymbol> m_resolved;
    std::vector<std::string> m_unresolved;
};

} // namespace quin::loader

#endif // QUIN_LOADER_DYNAMIC_LINKER_HPP
