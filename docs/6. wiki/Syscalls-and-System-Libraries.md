# 🏛️ Syscalls & System Libraries — Quin PS5 Emulator

## 1. FreeBSD / PS5 Syscall Dispatcher (`src/kernel/`)

### Syscall Architecture (`syscall_table.hpp`/`cpp`)
`SyscallDispatcher` maps FreeBSD / PS5 system call numbers and dispatches guest system requests using System V AMD64 ABI argument passing:
- `RAX`: Syscall Number / Return Value
- `RDI`: Argument 1
- `RSI`: Argument 2
- `RDX`: Argument 3
- `R10`: Argument 4
- `R8` : Argument 5
- `R9` : Argument 6

### Implemented Core Syscalls (23 Syscalls)
1. `SYS_exit` (#1): Terminates process execution with status code.
2. `SYS_read` (#3): Reads bytes from open VFS file handle into guest memory.
3. `SYS_write` (#4): Writes bytes from guest memory to stdout/stderr or open VFS file handle.
4. `SYS_open` (#5): Opens a file via VFS and returns file handle.
5. `SYS_close` (#6): Closes open VFS file handle.
6. `SYS_getpid` (#20): Returns guest process ID (1001).
7. `SYS_getuid` (#24) / `SYS_geteuid` (#25) / `SYS_getgid` (#47) / `SYS_getegid` (#43): Returns default PS5 user/group ID (1000).
8. `SYS_ioctl` (#54): Device I/O control stub (returns 0).
9. `SYS_munmap` (#73): Unmaps memory region.
10. `SYS_mprotect` (#74): Modifies page permissions.
11. `SYS_gettimeofday` (#116): Returns current time seconds and microseconds.
12. `SYS_writev` (#121): Vectored I/O write for `iovec` structures.
13. `SYS_stat` (#188): Stats file by path via VFS.
14. `SYS_fstat` (#189): Stats file by descriptor via VFS.
15. `SYS_lseek` (#199): Seeks file descriptor position via VFS.
16. `SYS_clock_gettime` (#232): Writes current system time into `timespec` struct.
17. `SYS_nanosleep` (#240): High-resolution host sleep.
18. `SYS_sigprocmask` (#340) & `SYS_sigaction` (#416): Signal action stubs.
19. `SYS_thr_exit` (#431) & `SYS_thr_self` (#432) & `SYS_thr_new` (#455): Thread creation and exit lifecycle.
20. `SYS_umtx_op` (#454): Futex / mutex operation handling (WAIT/WAKE).
21. `SYS_mmap` (#477): Allocates guest virtual memory.
22. `SYS_dynlib_dlsym` (#591): Dynamic symbol resolution stub.
23. `SYS_dynlib_load_prx` (#594) & `SYS_dynlib_get_proc_param` (#599): Module loading and process parameters.

---

## 2. System Library Modules (`src/kernel/modules/`)

### `ModuleManager` (`module_manager.hpp`/`cpp`)
Registers 6 core PS5 system library modules with `LibKernel`:

1. **`libSceLibcInternal`** (`sce_libc.cpp`): Standard C library memory operations (`sceLibcMalloc`, `sceLibcFree`, `sceLibcMemset`, `sceLibcMemcpy`).
2. **`libSceSystemService`** (`sce_system_service.cpp`): System environment parameters (`sceSystemServiceParamGetInt`, `sceSystemServiceHideSplashScreen`, `sceSystemServiceGetInitialUser`).
3. **`libSceUserService`** (`sce_user_service.cpp`): User profile services (`sceUserServiceInitialize`, `sceUserServiceGetInitialUser`, `sceUserServiceGetUserName`).
4. **`libSceAudioOut`** (`sce_audio_out.cpp`): Audio port management (`sceAudioOutInit`, `sceAudioOutOpen`, `sceAudioOutOutput`, `sceAudioOutSetVolume`, `sceAudioOutClose`).
5. **`libScePad`** (`sce_pad.cpp`): Controller input services (`scePadInit`, `scePadOpen`, `scePadReadState`, `scePadSetVibration`, `scePadSetLightBar`, `scePadClose`).
6. **`libSceNpTrophy`** (`sce_np_trophy.cpp`): Trophy system services (`sceNpTrophyCreateContext`, `sceNpTrophyCreateHandle`, `sceNpTrophyRegisterContext`, `sceNpTrophyUnlockTrophy`, `sceNpTrophyGetTrophyUnlockState`, `sceNpTrophyGetTrophyInfo`, `sceNpTrophyGetGameInfo`, `sceNpTrophyDestroyContext`, `sceNpTrophyDestroyHandle`).

### `LibKernel` System Library (`libkernel.hpp`/`cpp`)
Implements 20+ core kernel function stubs including `sceKernelLoadStartModule`, `sceKernelDlsym`, `sceKernelUsleep`, `sceKernelStat`, `sceKernelOpen/Close/Read/Lseek`, `sceKernelMmap`, `sceKernelMunmap`, `sceKernelCreateEqueue`, `sceKernelCreateEventFlag`, `sceKernelGetProcessTimeCounter`.
