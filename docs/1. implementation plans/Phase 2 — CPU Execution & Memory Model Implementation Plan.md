# Phase 2 — CPU Execution & Memory Model Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 2 — CPU Execution & Memory Model** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **Native Exception Interception**: On Windows, we will register a Vectored Exception Handler (`AddVectoredExceptionHandler`) and on POSIX `sigaction` (`SIGSEGV`, `SIGILL`) to capture guest memory violations and illegal instructions, translating them into structured guest stack traces without terminating the host process.
> - **Thread & TLS Mapping**: Guest thread contexts will map directly to host OS threads (`std::thread`), each maintaining its own register state, stack allocation with bottom guard pages, and PS5 ABI Thread-Local Storage (TLS) control block.

## Proposed Changes

---

### Memory Subsystem (`src/memory/`)

#### [MODIFY] [address_space.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/memory/address_space.hpp)
#### [MODIFY] [address_space.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/memory/address_space.cpp)
- Add `mprotect(guest_vaddr, size, permissions)` to dynamically alter page protection.
- Add `mmap(guest_vaddr, size, permissions, flags)` / `munmap(guest_vaddr, size)` aligned to 4KB page boundaries.
- Add Guard Page support (`PagePermission::None`) for catching stack overflow boundary violations.
- Support thread-safe page mapping operations using a mutex lock.

---

### CPU & Thread Subsystem (`src/cpu/`)

#### [NEW] [thread_context.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/thread_context.hpp)
#### [NEW] [thread_context.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/thread_context.cpp)
- Define `GuestThreadId` and `ThreadState` (`Uninitialized`, `Ready`, `Running`, `Waiting`, `Terminated`).
- Define `ThreadContext` containing `CpuRegisters`, thread stack range, guard page address, and TLS base pointer.

#### [NEW] [thread_manager.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/thread_manager.hpp)
#### [NEW] [thread_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/thread_manager.cpp)
- Multi-threaded guest thread manager spawning host `std::thread` workers.
- Thread-Local Storage (TLS) buffer allocator per thread matching PS5 ABI conventions.
- Thread synchronization primitives (join, suspend, resume, terminate).

#### [NEW] [exception_handler.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/exception_handler.hpp)
#### [NEW] [exception_handler.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/exception_handler.cpp)
- Windows VEH (`AddVectoredExceptionHandler`) and POSIX `sigaction` handler setup.
- Converts host faults (`EXCEPTION_ACCESS_VIOLATION`, `EXCEPTION_ILLEGAL_INSTRUCTION`, `SIGSEGV`, `SIGILL`) into structured guest diagnostic crash dumps (register dumps & call stack unwinding).

#### [MODIFY] [execution_engine.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/execution_engine.hpp)
#### [MODIFY] [execution_engine.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/execution_engine.cpp)
- Integrate multi-threaded thread manager and exception handler into `ExecutionEngine`.
- Implement `SYSCALL` (0x0F 0x05) trap handling and register argument extraction (`RAX` = syscall #, `RDI`, `RSI`, `RDX`, `R10`, `R8`, `R9`).

---

### GUI Debug Shell (`src/gui/`)

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp)
#### [MODIFY] [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Threads & TLS" ImGui view displaying active threads, states, stack ranges, and TLS bases.
- Add "Crash Log & Stack Trace" panel rendering structured exception reports.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include new CPU and Thread source files in `quin-core`.
- Add `tests/unit/test_cpu_memory.cpp` target to `quin-tests`.

#### [NEW] [test_cpu_memory.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_cpu_memory.cpp)
- Catch2 unit tests for:
  1. `mprotect` permission modification and `mmap`/`munmap`.
  2. Multi-threaded guest thread creation and TLS variable isolation.
  3. Guard page memory violation trap.
  4. Structured exception translation and guest stack trace unwinding.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update roadmap table marking Phase 2 as ✅ **Complete** and Phase 3 as 🟡 **Next** (to be performed right before git commit and push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe` and test thread creation, step-through execution, and memory protection toggling in the ImGui debug shell.
