# 📜 Roadmap & Development Status — Quin PS5 Emulator

## Roadmap Overview

Quin was developed across 11 structured engineering phases (Phases 0 through 10), advancing from foundational tooling to a full 60 FPS performance engine.

---

## Phase Status Summary

| Phase | Title | Core Goal | Verification Status |
| :--- | :--- | :--- | :---: |
| **Phase 0** | **Foundations & Tooling** | CMake build matrix, ImGui debug shell, spdlog logging | ✅ **Complete** |
| **Phase 1** | **Executable Loading** | 64-bit ELF/SELF parser, guest address space allocator, libkernel stubs | ✅ **Complete** |
| **Phase 2** | **CPU Execution & Memory Model** | Multi-threaded execution harness, TLS isolation, native VEH exception handling | ✅ **Complete** |
| **Phase 3** | **Syscalls & System Libraries** | FreeBSD syscall dispatcher, `libSceLibcInternal`, `libSceSystemService` stubs | ✅ **Complete** |
| **Phase 4** | **Filesystem & Decompression** | VFS mount table, SaveData container manager, Kraken/Oodle decompression | ✅ **Complete** |
| **Phase 5** | **GPU Command Processing** | GNM PM4 packet parser, Vulkan 1.3 backend, state-hash PSO cache | ✅ **Complete** |
| **Phase 6** | **Shader Recompilation** | RDNA2 ISA decoder, SPIR-V 1.5 bytecode emitter, persistent shader cache | ✅ **Complete** |
| **Phase 7** | **Audio Subsystem** | Tempest 3D Audio engine, 48 kHz multi-channel PCM output, `libSceAudioOut` | ✅ **Complete** |
| **Phase 8** | **Input Subsystem** | DualSense HID controller driver, normalized `PadState` maps, `libScePad` stubs | ✅ **Complete** |
| **Phase 9** | **Compatibility Expansion** | Per-title status matrix, automated stub triage logger, regression test runner | ✅ **Complete** |
| **Phase 10** | **Performance & 60 FPS Pass** | Persistent disk PSO cache, async shader compiler, frame-pacing & FSR scaling | ✅ **Complete** |

---

## Detailed Phase Breakdown

### Phase 0 — Foundations & Tooling
- **Deliverables**: Root `CMakeLists.txt`, `Dependencies.cmake` (FetchContent for `spdlog`, `Catch2`, `SDL2`, `Dear ImGui`), custom ImGui ring-buffer logging sink, initial `quin` main executable and static library `quin-core`.
- **Exit Criteria Met**: Clean C++20 compilation on MSVC 2022 / GCC 12 / Clang 15 with 0 warnings.

### Phase 1 — Executable Loading
- **Deliverables**: `SelfParser`, `ElfLoader`, `GuestAddressSpace`, `LibKernel` stubs.
- **Exit Criteria Met**: Successfully parses 64-bit ELF headers, maps `PT_LOAD` segments to guest memory, and bootstraps stack frames.

### Phase 2 — CPU Execution & Memory Model
- **Deliverables**: `ExecutionEngine`, `ThreadManager` with TLS isolation, `mmap`/`mprotect`/`munmap` implementations, Vectored Exception Handler (`ExceptionHandler`).
- **Exit Criteria Met**: Executes x86-64 instructions, handles multi-threaded TLS contexts, catches memory access violations cleanly.

### Phase 3 — Syscalls & System Libraries
- **Deliverables**: `SyscallDispatcher` handling FreeBSD ABI calls (`SYS_open`, `SYS_read`, `SYS_write`, `SYS_clock_gettime`, `SYS_mmap`, `SYS_thr_self`), `ModuleManager` registering `libSceLibcInternal`, `libSceSystemService`, `libSceUserService`.
- **Exit Criteria Met**: All Catch2 syscall tests passing cleanly (`9/9`).

### Phase 4 — Filesystem & Decompression
- **Deliverables**: `VirtualFilesystem` (VFS), `SaveDataManager`, `KrakenDecoder`.
- **Exit Criteria Met**: VFS path mapping `/app0/`, `/savedata/`, chunked Kraken asset decompression passing Catch2 tests (`12/12`).

### Phase 5 — GPU Command Processing
- **Deliverables**: `GnmCmdParser` for PM4 packets, `GnmToVulkanTranslator`, `VulkanBackend` with state-hash PSO cache.
- **Exit Criteria Met**: GNM surface to `VkFormat` translation, PM4 draw packet parsing passing Catch2 tests (`15/15`).

### Phase 6 — Shader Recompilation
- **Deliverables**: `ShaderRecompiler` translating RDNA2 bytecode into SPIR-V 1.5 binary modules, `ShaderCache` in-memory lookup.
- **Exit Criteria Met**: Vertex and Pixel shader recompilation passing Catch2 tests (`17/17`).

### Phase 7 — Audio Subsystem
- **Deliverables**: `AudioEngine` (48 kHz PCM routing), `libSceAudioOut` symbol stubs (`sceAudioOutInit`, `sceAudioOutOpen`, `sceAudioOutOutput`, `sceAudioOutSetVolume`).
- **Exit Criteria Met**: Dual audio port creation and PCM sample streaming passing Catch2 tests (`19/19`).

### Phase 8 — Input Subsystem
- **Deliverables**: `InputManager` DualSense driver, `libScePad` symbol stubs (`scePadInit`, `scePadOpen`, `scePadReadState`, `scePadSetVibration`, `scePadSetLightBar`).
- **Exit Criteria Met**: Pad state reading, RGB lightbar control, rumble vibration passing Catch2 tests (`21/21`).

### Phase 9 — Compatibility Expansion
- **Deliverables**: `TitleDatabase` status tracker, `CompatTriage` missing symbol logger, `compatibility.md` report, automated regression test runner.
- **Exit Criteria Met**: Title database lookup and triage call counting passing Catch2 tests (`23/23`).

### Phase 10 — Performance & 60 FPS Pass
- **Deliverables**: `PsoDiskCache` binary serialization, `AsyncShaderCompiler` multi-threaded worker pool, `FramePacingRegulator` (30/60/Unlocked FPS, FSR dynamic resolution scaling).
- **Exit Criteria Met**: Disk PSO cache persistence, async shader compilation, frame pacing calculation passing Catch2 tests (`26/26`).
