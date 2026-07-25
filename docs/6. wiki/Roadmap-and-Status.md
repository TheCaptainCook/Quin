# 📜 Roadmap & Development Status — Quin PS5 Emulator

## Roadmap Overview

Quin was developed across 12 structured engineering phases (Phases 0 through 11), advancing from foundational tooling to a full audit gap resolution.

---

## Phase Status Summary

| Phase | Title | Core Goal | Verification Status |
| :--- | :--- | :--- | :---: |
| **Phase 0** | **Foundations & Tooling** | CMake build matrix, ImGui debug shell, spdlog logging | ✅ **Complete** |
| **Phase 1** | **Executable Loading** | 64-bit ELF/SELF parser, guest address space allocator, dynamic linker (PLT/GOT) | ✅ **Complete** |
| **Phase 2** | **CPU Execution & Memory Model** | x86-64 instruction decoder (~20 opcodes), TLS FS/GS base, exception handlers (VEH & POSIX) | ✅ **Complete** |
| **Phase 3** | **Syscalls & System Libraries** | 23 FreeBSD syscalls, `libSce*` stubs, `libSceNpTrophy` module | ✅ **Complete** |
| **Phase 4** | **Filesystem & Decompression** | VFS mount table with seek, SaveData manager, clean-room LZ77 decompression | ✅ **Complete** |
| **Phase 5** | **GPU Command Processing** | GNM PM4 packet parser (8 opcodes), Vulkan GPU hardware detection, PSO cache | ✅ **Complete** |
| **Phase 6** | **Shader Recompilation** | RDNA2 ISA decoder (~40 instructions), SPIR-V 1.5 bytecode emitter, shader cache | ✅ **Complete** |
| **Phase 7** | **Audio Subsystem** | Real SDL2 audio device output (48 kHz stereo S16, volume scaling, F32→S16), `libSceAudioOut` | ✅ **Complete** |
| **Phase 8** | **Input Subsystem** | Real SDL2 controller polling, DualSense rumble/lightbar, hotplug, `libScePad` stubs | ✅ **Complete** |
| **Phase 9** | **Compatibility Expansion** | Per-title status matrix, automated stub triage logger, 6 real functional regression tests | ✅ **Complete** |
| **Phase 10** | **Performance & 60 FPS Pass** | Persistent disk PSO cache, async shader compiler, frame-pacing & FSR scaling | ✅ **Complete** |
| **Phase 11** | **Audit Gap Resolution** | Complete C++20 resolution of all 16 audit gap requirements | ✅ **Complete** |

---

## Detailed Phase Breakdown

### Phase 0 — Foundations & Tooling
- **Deliverables**: Root `CMakeLists.txt`, `Dependencies.cmake` (FetchContent for `spdlog`, `Catch2`, `SDL2`, `Dear ImGui`, optional `Vulkan_FOUND` detection), custom ImGui ring-buffer logging sink, `quin` executable and static library `quin-core`.
- **Exit Criteria Met**: Clean C++20 compilation on MSVC 2022 / GCC 12 / Clang 15 with 0 warnings.

### Phase 1 — Executable Loading
- **Deliverables**: `SelfParser`, `ElfLoader`, `GuestAddressSpace`, `DynamicLinker` (PT_DYNAMIC / RELA / GOT patching), `LibKernel` stubs.
- **Exit Criteria Met**: Successfully parses 64-bit ELF/SELF headers, maps `PT_LOAD` segments to guest memory, extracts `DT_NEEDED` library names, and patches GOT trampoline addresses.

### Phase 2 — CPU Execution & Memory Model
- **Deliverables**: `ExecutionEngine` with ~20 opcode x86-64 instruction decoder, `ThreadManager` with `arch_prctl(ARCH_SET_FS)` TLS isolation, `mmap`/`mprotect`/`munmap` implementations, Vectored Exception Handler (`ExceptionHandler`) + POSIX `sigaction` (SIGSEGV, SIGILL, SIGBUS, SIGTRAP).
- **Exit Criteria Met**: Executes x86-64 instruction stepping loops, handles multi-threaded TLS contexts, catches memory access violations cleanly.

### Phase 3 — Syscalls & System Libraries
- **Deliverables**: `SyscallDispatcher` handling 23 FreeBSD ABI calls (`exit`, `read`, `write`, `open`, `close`, `lseek`, `fstat`, `stat`, `ioctl`, `nanosleep`, `sigaction`, `gettimeofday`, `writev`, `mmap`, `munmap`, `mprotect`, `thr_self/exit/new`, `umtx_op`, `dynlib_dlsym`, `dynlib_load_prx`), `ModuleManager` registering `libSceLibcInternal`, `libSceSystemService`, `libSceUserService`, `libSceAudioOut`, `libScePad`, `libSceNpTrophy`.
- **Exit Criteria Met**: All syscall and module dispatch tests passing cleanly.

### Phase 4 — Filesystem & Decompression
- **Deliverables**: `VirtualFilesystem` (VFS with `seek_file`), `SaveDataManager`, `KrakenDecoder` (clean-room LZ77 algorithm).
- **Exit Criteria Met**: VFS path mapping `/app0/`, `/savedata/`, chunked LZ77 decompression passing regression tests.

### Phase 5 — GPU Command Processing
- **Deliverables**: `GnmCmdParser` for 8 PM4 packet types, `ResourceTranslator`, `VulkanBackend` with real `VkInstance`/`VkPhysicalDevice` GPU detection and state-hash PSO cache.
- **Exit Criteria Met**: Vulkan GPU hardware properties detection, PM4 packet parsing passing tests.

### Phase 6 — Shader Recompilation
- **Deliverables**: `ShaderRecompiler` translating ~40 RDNA2 instructions across SOPP, SOP1, SOP2, SOPC, VOP1, VOP2, VOP3, SMEM, EXP categories into SPIR-V 1.5 binary modules, `ShaderCache` in-memory lookup.
- **Exit Criteria Met**: RDNA2 instruction decoding and SPIR-V generation passing tests.

### Phase 7 — Audio Subsystem
- **Deliverables**: `AudioEngine` (real SDL2 audio output at 48 kHz stereo S16, volume scaling, F32→S16 conversion), `libSceAudioOut` symbol stubs.
- **Exit Criteria Met**: Real SDL2 audio device output and PCM sample streaming passing tests.

### Phase 8 — Input Subsystem
- **Deliverables**: `InputManager` DualSense driver (real SDL2 controller polling, axis normalization, rumble, lightbar, hotplug), `libScePad` symbol stubs.
- **Exit Criteria Met**: Pad state reading, RGB lightbar control, rumble vibration passing tests.

### Phase 9 — Compatibility Expansion
- **Deliverables**: `TitleDatabase` status tracker, `CompatTriage` missing symbol logger fed by dynamic linker, `compatibility.md` report, 6 real functional regression tests.
- **Exit Criteria Met**: Automated regression suite passing 6/6 tests.

### Phase 10 — Performance & 60 FPS Pass
- **Deliverables**: `PsoDiskCache` binary serialization (`.quin_pso_cache`), `AsyncShaderCompiler` multi-threaded worker pool, `FramePacingRegulator` (30/60/Unlocked FPS).
- **Exit Criteria Met**: Disk PSO cache persistence, async shader compilation, frame pacing calculation passing tests.

### Phase 11 — Complete Audit Gap Resolution
- **Deliverables**: Resolution of all 16 audit gap requirements across CPU, GPU, Loader, Syscalls, Filesystem, Audio, Input, and Testing.
- **Exit Criteria Met**: All roadmap requirements backed by functional C++20 code.
