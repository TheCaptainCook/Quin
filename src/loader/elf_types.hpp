#ifndef QUIN_LOADER_ELF_TYPES_HPP
#define QUIN_LOADER_ELF_TYPES_HPP

#include <cstdint>
#include <cstddef>

namespace quin::loader {

// 64-bit ELF Types
using Elf64_Addr   = uint64_t;
using Elf64_Off    = uint64_t;
using Elf64_Half   = uint16_t;
using Elf64_Word   = uint32_t;
using Elf64_Sword  = int32_t;
using Elf64_Xword  = uint64_t;
using Elf64_Sxword = int64_t;

// ELF Identifiers
constexpr size_t EI_NIDENT = 16;

constexpr uint8_t ELF_MAG0 = 0x7f;
constexpr uint8_t ELF_MAG1 = 'E';
constexpr uint8_t ELF_MAG2 = 'L';
constexpr uint8_t ELF_MAG3 = 'F';

constexpr uint8_t ELFCLASS64 = 2;
constexpr uint8_t ELFDATA2LSB = 1; // Little-endian

// ELF File Types
constexpr Elf64_Half ET_EXEC = 2;
constexpr Elf64_Half ET_DYN  = 3;

// Machine Architecture (x86-64)
constexpr Elf64_Half EM_X86_64 = 62;

// Program Header Segment Types
constexpr Elf64_Word PT_NULL    = 0;
constexpr Elf64_Word PT_LOAD    = 1;
constexpr Elf64_Word PT_DYNAMIC = 2;
constexpr Elf64_Word PT_INTERP  = 3;
constexpr Elf64_Word PT_NOTE    = 4;
constexpr Elf64_Word PT_SHLIB   = 5;
constexpr Elf64_Word PT_PHDR    = 6;
constexpr Elf64_Word PT_TLS     = 7;

// Program Header Segment Flags
constexpr Elf64_Word PF_X = 0x1; // Execute
constexpr Elf64_Word PF_W = 0x2; // Write
constexpr Elf64_Word PF_R = 0x4; // Read

// Dynamic Array Tags
constexpr Elf64_Sxword DT_NULL     = 0;
constexpr Elf64_Sxword DT_NEEDED   = 1;
constexpr Elf64_Sxword DT_PLTRELSZ = 2;
constexpr Elf64_Sxword DT_STRTAB   = 5;
constexpr Elf64_Sxword DT_SYMTAB   = 6;
constexpr Elf64_Sxword DT_RELA     = 7;
constexpr Elf64_Sxword DT_RELASZ   = 8;
constexpr Elf64_Sxword DT_RELAENT  = 9;

// ELF64 Header
#pragma pack(push, 1)
struct Elf64_Ehdr {
    uint8_t    e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off  e_phoff;
    Elf64_Off  e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
};

// ELF64 Program Header
struct Elf64_Phdr {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off  p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
};

// ELF64 Section Header
struct Elf64_Shdr {
    Elf64_Word  sh_name;
    Elf64_Word  sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr  sh_addr;
    Elf64_Off   sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word  sh_link;
    Elf64_Word  sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
};

// ELF64 Symbol Table Entry
struct Elf64_Sym {
    Elf64_Word  st_name;
    uint8_t     st_info;
    uint8_t     st_other;
    Elf64_Half  st_shndx;
    Elf64_Addr  st_value;
    Elf64_Xword st_size;
};

// ELF64 Relocation Entry with Addend
struct Elf64_Rela {
    Elf64_Addr   r_offset;
    Elf64_Xword  r_info;
    Elf64_Sxword r_addend;
};

// ELF64 Dynamic Structure
struct Elf64_Dyn {
    Elf64_Sxword d_tag;
    union {
        Elf64_Xword d_val;
        Elf64_Addr  d_ptr;
    } d_un;
};
#pragma pack(pop)

inline uint32_t ELF64_R_SYM(Elf64_Xword info) { return static_cast<uint32_t>(info >> 32); }
inline uint32_t ELF64_R_TYPE(Elf64_Xword info) { return static_cast<uint32_t>(info & 0xFFFFFFFF); }

} // namespace quin::loader

#endif // QUIN_LOADER_ELF_TYPES_HPP
