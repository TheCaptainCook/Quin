# 🏛️ Syscalls & System Libraries — Quin PS5 Emulator

## 1. FreeBSD / PS5 Syscall Architecture (`src/kernel/`)

### Syscall Dispatcher (`syscall_table.hpp`/`cpp`)
Quin implements a FreeBSD 12 / PS5 kernel syscall dispatcher. Guest x86-64 code triggers syscalls via `SYSCALL` instructions.

#### ABI Register Mapping (System V AMD64 ABI)
- **Syscall Number**: Loaded in `RAX`
- **Argument 1**: `RDI`
- **Argument 2**: `RSI`
- **Argument 3**: `RDX`
- **Argument 4**: `RCX` (or `R10`)
- **Argument 5**: `R8`
- **Argument 6**: `R9`
- **Return Value**: Returned in `RAX` (0 = Success, >0 = Error code)

### Standard Implemented FreeBSD Syscalls
- `SYS_open` (#5): Opens guest file via VFS layer.
- `SYS_close` (#6): Closes active VFS file handle.
- `SYS_read` (#3): Reads bytes from file descriptor into guest memory.
- `SYS_write` (#4): Writes bytes from guest memory to file descriptor / stdout log.
- `SYS_mmap` (#477): Allocates guest virtual memory pages.
- `SYS_munmap` (#73): Frees allocated guest memory pages.
- `SYS_mprotect` (#74): Modifies guest memory page permissions.
- `SYS_clock_gettime` (#232): Returns high-resolution nanosecond clock timestamps.
- `SYS_thr_self` (#432): Returns active thread ID (`TID`).
- `SYS_dynlib_load_prx` (#594): Loads dynamic PRX library module stubs.

---

## 2. System Module Manager (`module_manager.hpp`/`cpp`)

`ModuleManager` coordinates registration and stub dispatch for core PS5 `libSce*` system libraries:

```
+-------------------------------------------------------------------+
|                         ModuleManager                             |
|                                                                   |
|  +-------------------+  +-------------------+  +---------------+  |
|  | libSceLibcInternal|  |libSceSystemService|  |libSceUserService|  |
|  +-------------------+  +-------------------+  +---------------+  |
|            |                      |                    |          |
|            v                      v                    v          |
|  +-------------------+  +-------------------+                     |
|  |   libSceAudioOut  |  |     libScePad     |                     |
|  +-------------------+  +-------------------+                     |
+-------------------------------------------------------------------+
```

---

## 3. Registered System Modules

### `libkernel` (`src/kernel/libkernel.hpp`/`cpp`)
Provides symbol stubs for core OS functions:
- `sceKernelAllocateMainDirectMemory`
- `sceKernelMapDirectMemory`
- `sceKernelCreateThread`
- `sceKernelStartThread`

### `libSceLibcInternal` (`src/kernel/modules/sce_libc.cpp`)
Implements standard C runtime stubs:
- `malloc`, `free`, `calloc`, `realloc`, `memcpy`, `memset`, `memcmp`, `strlen`, `strcpy`, `snprintf`.

### `libSceSystemService` (`src/kernel/modules/sce_system_service.cpp`)
Provides system state stubs:
- `sceSystemServiceInitialize`, `sceSystemServiceHideSplashScreen`, `sceSystemServiceGetEvent`.

### `libSceUserService` (`src/kernel/modules/sce_user_service.cpp`)
Provides user profile stubs:
- `sceUserServiceInitialize`, `sceUserServiceGetInitialUser`, `sceUserServiceGetUserName`.

### `libSceAudioOut` (`src/kernel/modules/sce_audio_out.cpp`)
Provides audio output stubs:
- `sceAudioOutInit`, `sceAudioOutOpen`, `sceAudioOutOutput`, `sceAudioOutSetVolume`, `sceAudioOutClose`.

### `libScePad` (`src/kernel/modules/sce_pad.cpp`)
Provides DualSense controller stubs:
- `scePadInit`, `scePadOpen`, `scePadReadState`, `scePadSetVibration`, `scePadSetLightBar`, `scePadClose`.
