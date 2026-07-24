# 🧠 CPU & Memory Subsystem — Quin PS5 Emulator

## 1. Executable Loading (`src/loader/`)

### ELF64 & SELF Data Structures (`elf_types.hpp`)
Quin implements clean-room 64-bit Executable and Linkable Format (ELF) structures:
- `Elf64_Ehdr`: Identifies 64-bit Little-Endian x86-64 executable type (`ET_EXEC`/`ET_DYN`).
- `Elf64_Phdr`: Program headers specifying segment types (`PT_LOAD`, `PT_DYNAMIC`, `PT_INTERP`, `PT_TLS`).

### SELF Parser (`self_parser.hpp`/`cpp`)
- `SelfParser::parse_buffer()` reads signed ELF container headers.
- Extracts decrypted payload blocks and verifies 64-bit ELF magic (`0x7F 'E' 'L' 'F'`).

### ELF Loader (`elf_loader.hpp`/`cpp`)
- `ElfLoader::load()` allocates page-aligned virtual memory regions via `GuestAddressSpace`.
- Copies `PT_LOAD` segment payload bytes to virtual addresses (`p_vaddr`).
- Sets segment memory permissions (`PF_R`, `PF_W`, `PF_X`) mapped to `PROT_READ`, `PROT_WRITE`, `PROT_EXEC`.
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
- Allocates isolated Thread-Local Storage (TLS) data blocks per thread.
- Maintains 64-bit register state (`rip`, `rsp`, `rbp`, `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`-`r15`, `rflags`).

### Execution Engine (`execution_engine.hpp`/`cpp`)
- `ExecutionEngine` bootstraps execution at `e_entry`.
- Dispatches SYSCALL instructions to `SyscallDispatcher`.
- Features single-step instruction execution (`step()`), execution pause, and state reset.

### Native Exception Interception (`exception_handler.hpp`/`cpp`)
- `ExceptionHandler` initializes Windows Vectored Exception Handlers (VEH) and POSIX signal handlers (`SIGSEGV`, `SIGBUS`).
- Catches access violations when guest code attempts illegal memory access, logging exact faulting address without crashing host emulator.
