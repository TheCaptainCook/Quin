# Phase 1 Implementation Plan — Executable Loading & Process Bootstrap

Implement **Phase 1** of `docs/ps5-emultor.md` for **Quin**: executable parsing, guest virtual address space allocation, `PT_LOAD` segment mapping, minimal `libkernel` symbol resolution, and an execution trap harness.

---

## User Review Required

> [!IMPORTANT]
> **Executable Format Scope**: Phase 1 supports standard 64-bit x86-64 ELF binaries and SELF (Signed ELF) header unwrapping built strictly clean-room from public specifications.
> 
> **Memory Allocation**: Uses page-aligned OS virtual allocation (`VirtualAlloc` on Windows / `mmap` on POSIX) to mirror the 64-bit guest address space permissions (`READ`, `WRITE`, `EXEC`).

---

## Open Questions

> [!NOTE]
> **libkernel Stub Scope**: For Phase 1, stubbed functions will return `0` (success) or log a descriptive `QUIN_LOG_WARN` warning when hit, providing enough PLT/GOT resolution for homebrew entry points to execute without crashing.

---

## Proposed Changes

### Loader & Binary Parser Subsystem

#### [NEW] [src/loader/elf_types.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/elf_types.hpp)
- Defines standard 64-bit ELF structures (`Elf64_Ehdr`, `Elf64_Phdr`, `Elf64_Shdr`, `Elf64_Sym`, `Elf64_Rela`, segment types `PT_LOAD`, `PT_DYNAMIC`, `PT_TLS`, `PT_GNU_STACK`, dynamic tags `DT_NEEDED`, `DT_RELA`, `DT_STRTAB`, `DT_SYMTAB`).

#### [NEW] [src/loader/self_parser.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/self_parser.hpp)
#### [NEW] [src/loader/self_parser.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/self_parser.cpp)
- Header validation (ELF magic `\x7fELF` and SELF headers).
- Extracts program header segments, entry point virtual address, section headers, and dynamic symbol tables.

#### [NEW] [src/loader/elf_loader.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/elf_loader.hpp)
#### [NEW] [src/loader/elf_loader.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/elf_loader.cpp)
- Orchestrates loading: validates file, requests guest memory allocation, maps `PT_LOAD` segments, zeroes BSS sections, sets up guest stack space.

---

### Guest Memory Management

#### [NEW] [src/memory/address_space.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/memory/address_space.hpp)
#### [NEW] [src/memory/address_space.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/memory/address_space.cpp)
- `GuestAddressSpace` class:
  - Allocates page-aligned memory buffers via OS virtual memory.
  - Controls page permissions (`PageRead`, `PageWrite`, `PageExec`).
  - Maps guest virtual addresses to host memory pointers.

---

### System Library Stubs & Execution Harness

#### [NEW] [src/kernel/libkernel.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/libkernel.hpp)
#### [NEW] [src/kernel/libkernel.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/libkernel.cpp)
- Symbol lookup table for `libkernel.sprx` / `libkernel_web.sprx` exports:
  - Core stubs: `sceKernelExitProcess`, `sceKernelWrite`, `__stack_chk_fail`, `sceKernelGetProcessTime`, `sceKernelMmap`, `sceKernelMunmap`.
  - Fallback handler for unmapped GOT/PLT symbols.

#### [NEW] [src/cpu/execution_engine.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/execution_engine.hpp)
#### [NEW] [src/cpu/execution_engine.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/execution_engine.cpp)
- Guest thread execution context:
  - Manages guest registers, stack pointer, instruction pointer.
  - Provides a trap handler trapping guest exceptions / traps to `spdlog` structured diagnostics.

---

### GUI Debug Shell Integration & Build System

#### [MODIFY] [src/gui/debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Integrates `SelfParser` and `ElfLoader` into the "Load ELF / SELF..." file loader interface.
- Displays parsed segment headers (`PT_LOAD` addresses, sizes, memory permissions) and symbol tables live in the UI.

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Adds loader, memory, kernel, and cpu source files to `quin-core` target.

#### [NEW] [tests/unit/test_elf_loader.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_elf_loader.cpp)
- Unit test constructing a synthetic ELF64 binary buffer in memory, parsing headers, verifying segment mapping into `GuestAddressSpace`, and testing `libkernel` symbol resolution.

---

## Verification Plan

### Automated Tests
- Build `quin-core`, `quin`, and `quin-tests` (`cmake --build build --config Release`).
- Run expanded test suite via `ctest --test-dir build -C Release --output-on-failure`.

### Manual Verification
- Launch `quin.exe` debug shell.
- Click "Load Sample ELF File" / load an ELF binary.
- Verify segment table listing (`PT_LOAD` virtual addresses, memory protections) and `libkernel` symbol resolution in the Debug Shell UI.
