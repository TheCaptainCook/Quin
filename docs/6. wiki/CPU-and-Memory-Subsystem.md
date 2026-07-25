# 🧠 CPU & Memory Subsystem — Quin PS5 Emulator

## 1. Executable Loading & Dynamic Linking (`src/loader/`)

### ELF64 & SELF Data Structures (`elf_types.hpp`)
Quin implements clean-room 64-bit Executable and Linkable Format (ELF) structures:
- `Elf64_Ehdr`: Identifies 64-bit Little-Endian x86-64 executable type (`ET_EXEC`/`ET_DYN`).
- `Elf64_Phdr`: Program headers specifying segment types (`PT_LOAD`, `PT_DYNAMIC`, `PT_INTERP`, `PT_TLS`).
- `Elf64_Dyn` & `Elf64_Rela`: Dynamic section structure and relocation entries.

### SELF Parser (`self_parser.hpp`/`cpp`)
- `SelfParser::parse_buffer()` reads signed ELF container headers.
- Extracts decrypted payload blocks, verifies 64-bit ELF magic (`0x7F 'E' 'L' 'F'`), and parses `PT_DYNAMIC` to collect `DT_NEEDED` library dependencies.

### Dynamic Linker (`dynamic_linker.hpp`/`cpp`)
- `DynamicLinker::link()` parses dynamic tables, iterates over `RELA` / `JMPREL` relocation entries, matches symbol strings in `DT_STRTAB`/`DT_SYMTAB` against registered `LibKernel` stubs, writes trampoline addresses into GOT entries, and reports unresolved symbols to `CompatTriage`.

### ELF Loader (`elf_loader.hpp`/`cpp`)
- `ElfLoader::load()` allocates page-aligned virtual memory regions via `GuestAddressSpace`.
- Copies `PT_LOAD` segment payload bytes to virtual addresses (`p_vaddr`).
- Sets segment memory permissions (`PF_R`, `PF_W`, `PF_X`) mapped to `PagePermission`.
- Bootstraps guest execution stacks (`stack_top = 0x00007FFFF0000000ULL`).

---

## 2. Guest Memory Manager (`src/memory/`)

### Address Space Layout (`address_space.hpp`/`cpp`)
- `GuestAddressSpace` manages virtual memory matching PS5 64-bit address conventions:
  - **Code/Data Segments**: `0x0000000000400000` — `0x0000000080000000`
  - **Heap Region (`mmap`)**: `0x0000000080000000` — `0x00007FF000000000`
  - **Stack Region**: `0x00007FFF00000000` — `0x00007FFFF0000000`

### Memory Operations (`mmap`, `mprotect`, `munmap`)
- `mmap(addr, length, prot, flags)`: Allocates page-aligned memory blocks (4 KB pages).
- `mprotect(addr, length, prot)`: Modifies page protection flags dynamically.
- `munmap(addr, length)`: Frees mapped memory regions.
- **Guard Pages**: Places unmapped guard pages at stack and heap boundaries to trap out-of-bounds guest access.

---

## 3. CPU Execution & Threading (`src/cpu/`)

### Thread Context Manager (`thread_manager.hpp`/`cpp`)
- `ThreadManager` manages guest OS thread lifecycles (`Ready`, `Running`, `Waiting`, `Terminated`).
- Spawns host worker threads running guest instruction stepping.
- Allocates isolated Thread-Local Storage (TLS) data blocks per thread (self-pointer at offset 0, TID at offset 8).
- Sets FS base register via `arch_prctl(ARCH_SET_FS)` on Linux x86-64.
- Maintains 64-bit register state (`rip`, `rsp`, `rbp`, `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`-`r15`, `rflags`).

### Execution Engine (`execution_engine.hpp`/`cpp`)
- `ExecutionEngine` bootstraps execution at `e_entry`.
- Features an x86-64 instruction decoder handling ~20 opcodes: `MOV`, `PUSH`, `POP`, `CALL`, `JMP`, `Jcc` (all 16 condition codes), `ADD`, `SUB`, `XOR`, `CMP`, `TEST`, `LEA`, `LEAVE`, `INT3`, `HLT`, `SYSCALL`.
- Evaluates RFLAGS flags (`ZF`, `SF`, `CF`, `OF`) and decodes variable-length x86-64 instructions with REX prefix support.
- Dispatches SYSCALL instructions to `SyscallDispatcher`.

### Native Exception Interception (`exception_handler.hpp`/`cpp`)
- `ExceptionHandler` initializes Windows Vectored Exception Handlers (VEH) and POSIX signal handlers (`SIGSEGV`, `SIGILL`, `SIGBUS`, `SIGTRAP`).
- Extracts fault address and RIP from `ucontext_t` / exception pointers, logging diagnostics without crashing the host.
