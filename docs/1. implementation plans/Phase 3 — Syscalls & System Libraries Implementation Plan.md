# Phase 3 — Syscalls & System Libraries Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 3 — Syscalls & System Libraries** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **FreeBSD / PS5 Syscall ABI**: Syscalls will be intercepted from `SYSCALL` instructions (`0x0F 0x05`) using the standard PS5 x86-64 register convention (`RAX` = syscall #, `RDI`, `RSI`, `RDX`, `R10`, `R8`, `R9`).
> - **Module Resolution**: High-frequency system libraries (`libSceLibcInternal`, `libSceSystemService`, `libSceUserService`, `libScePad`, `libSceVideoOut`) will be registered with fallback warning handlers to capture and log every unimplemented call hit.

## Proposed Changes

---

### Syscall Subsystem (`src/kernel/`)

#### [NEW] [syscall_table.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/syscall_table.hpp)
#### [NEW] [syscall_table.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/syscall_table.cpp)
- Define FreeBSD/PS5 syscall numbers (`SYS_exit`, `SYS_read`, `SYS_write`, `SYS_open`, `SYS_close`, `SYS_mmap`, `SYS_munmap`, `SYS_mprotect`, `SYS_clock_gettime`, `SYS_thr_new`, `SYS_thr_exit`, `SYS_thr_self`, `SYS_dynlib_load_prx`, `SYS_dynlib_dlsym`).
- Implement `SyscallDispatcher` executing host C++ handlers and returning status code in `RAX`.

---

### `libSce*` System Modules (`src/kernel/modules/`)

#### [NEW] [sce_libc.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_libc.hpp) & [sce_libc.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_libc.cpp)
- Implement `libSceLibcInternal` stubs (`malloc`, `free`, `memset`, `memcpy`, `snprintf`, `puts`, `abort`).

#### [NEW] [sce_system_service.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_system_service.hpp) & [sce_system_service.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_system_service.cpp)
- Implement `libSceSystemService` stubs (`sceSystemServiceParamGetInt`, `sceSystemServiceHideSplashScreen`).

#### [NEW] [sce_user_service.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_user_service.hpp) & [sce_user_service.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_user_service.cpp)
- Implement `libSceUserService` stubs (`sceUserServiceInitialize`, `sceUserServiceGetInitialUser`, `sceUserServiceGetUserName`).

#### [NEW] [module_manager.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/module_manager.hpp) & [module_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/module_manager.cpp)
- System module manager registering all `libSce*` libraries into `LibKernel`.

---

### Core Integration & Debug UI (`src/cpu/`, `src/gui/`)

#### [MODIFY] [execution_engine.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/execution_engine.cpp)
- Connect `ExecutionEngine` `SYSCALL` instruction handling to `SyscallDispatcher`.

#### [MODIFY] [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Syscalls & System Modules" panel in ImGui Debug Shell showing registered syscalls and call metrics.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include new kernel module source files in `quin-core`.
- Add `tests/unit/test_syscalls_modules.cpp` target to `quin-tests`.

#### [NEW] [test_syscalls_modules.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_syscalls_modules.cpp)
- Catch2 unit tests for FreeBSD syscalls and `libSce*` symbol resolutions.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 3 as ✅ **Complete** and Phase 4 as 🟡 **Next** (before git push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, view registered FreeBSD syscalls and `libSce*` system libraries in the ImGui debug shell.
