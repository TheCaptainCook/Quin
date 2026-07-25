# Implementation Plan — Complete Audit Gap Resolution

Address all 16 items identified as not implemented or partially implemented in the [ps5-emultor.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/docs/4.%20instructions/ps5-emultor.md) audit.

## User Review Required

> [!IMPORTANT]
> **SDL2 vs SDL3**: The roadmap says "SDL3" but the entire codebase (ImGui backends, main.cpp, audio, input) uses SDL2. Migrating to SDL3 would require rewriting every SDL call and switching ImGui backends (`imgui_impl_sdl3.h`). **This plan keeps SDL2** since SDL3 is not yet widely stable and the ImGui SDL3 backend has known issues. If you want SDL3, let me know and I'll add that as a separate migration phase.

> [!IMPORTANT]
> **vcpkg vs FetchContent**: The roadmap says "vcpkg or Conan" but FetchContent is working and is simpler for contributors. **This plan keeps FetchContent** but adds the missing deps (Vulkan SDK, SPIRV-Tools). If you want vcpkg, let me know.

> [!IMPORTANT]  
> **Kraken/Oodle**: The real Kraken codec is proprietary (RAD Game Tools). This plan implements a clean-room **LZ77 decompression algorithm** that handles the KRAK-magic chunks. For real PS5 game data, Oodle licensing would still be needed — this is noted in the roadmap itself.

## Open Questions

> [!NOTE]
> No blocking questions — all items have clear implementation paths based on the roadmap spec.

---

## Proposed Changes

### Group 1: CPU Execution & Exception Handling (Phase 2)

Expand the 3-opcode interpreter into a proper x86-64 instruction decoder with ~20 common opcodes, add Linux/macOS signal handlers, and fix TLS FS/GS base setup.

---

#### [MODIFY] [execution_engine.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/execution_engine.hpp)
- Add `decode_instruction()` and `get_instruction_length()` helper methods

#### [MODIFY] [execution_engine.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/execution_engine.cpp)
- Replace the 3-opcode `step()` with a proper x86-64 decoder handling: `NOP`, `RET`, `SYSCALL`, `MOV reg,imm`, `MOV reg,reg`, `PUSH reg`, `POP reg`, `CALL rel32`, `JMP rel32/rel8`, `ADD`, `SUB`, `XOR`, `CMP`, `TEST`, `JZ/JNZ/JE/JNE`, `LEA`, `INT3`, `HLT`
- Add proper variable-length instruction decoding (REX prefix, ModR/M byte awareness)
- Log decoded instruction mnemonics for debugging

#### [MODIFY] [exception_handler.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/exception_handler.cpp)
- Add Linux `sigaction` handlers for `SIGSEGV`, `SIGILL`, `SIGBUS`, `SIGTRAP` in the `#else` block
- Add macOS support (same POSIX path)
- Extract fault address and RIP from `ucontext_t`

#### [MODIFY] [thread_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/cpu/thread_manager.cpp)
- Replace placeholder `sleep_for(10ms)` thread body with actual guest code execution loop (step-through using the same instruction decoder)
- Add `set_tls_base()` that calls `arch_prctl(ARCH_SET_FS)` on Linux or uses Windows thread context for FS/GS base

---

### Group 2: Syscalls & System Libraries (Phase 3)

Expand from 8 to ~20 core syscalls; add trophy module; improve Sce* module stubs.

---

#### [MODIFY] [syscall_table.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/syscall_table.hpp)
- Add missing syscall number constants: `SYS_lseek`, `SYS_ioctl`, `SYS_fstat`, `SYS_nanosleep`, `SYS_sigaction`, `SYS_getuid`, `SYS_gettimeofday`, `SYS_writev`, `SYS_munmap`, `SYS_mprotect`, `SYS_umtx_op`

#### [MODIFY] [syscall_table.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/syscall_table.cpp)
- Implement: `sys_lseek` (VFS seek), `sys_fstat` (file size via VFS), `sys_ioctl` (stub return 0), `sys_nanosleep` (host sleep), `sys_sigaction` (stub), `sys_getuid/getgid` (return 1000), `sys_gettimeofday`, `sys_writev`, `sys_munmap`, `sys_mprotect`, `sys_thr_new` (delegate to thread manager), `sys_thr_exit`, `sys_umtx_op` (basic futex stub)

#### [NEW] [sce_np_trophy.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_np_trophy.cpp)
- Stub `sceNpTrophyCreateContext`, `sceNpTrophyCreateHandle`, `sceNpTrophyUnlockTrophy`, `sceNpTrophyGetTrophyInfo` — all return success (0)

#### [NEW] [sce_np_trophy.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_np_trophy.hpp)
- Header for trophy module registration function

#### [MODIFY] [module_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/module_manager.cpp)
- Register `libSceNpTrophy` module

#### [MODIFY] [libkernel.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/libkernel.cpp)
- Add more default stubs: `sceKernelLoadStartModule`, `sceKernelDlsym`, `sceKernelGetFsSandboxRandomWord`, `sceKernelStat`, `sceKernelUsleep`

---

### Group 3: Loader & Dynamic Linking (Phase 1)

Add basic PLT/GOT resolution connecting ELF imports to libkernel stubs.

---

#### [NEW] [dynamic_linker.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/dynamic_linker.hpp)
- `DynamicLinker` class: parse `PT_DYNAMIC` segment, extract `DT_NEEDED`, `DT_STRTAB`, `DT_SYMTAB`, `DT_RELA`; resolve imported symbols against libkernel stubs; patch GOT entries

#### [NEW] [dynamic_linker.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/dynamic_linker.cpp)
- Parse the dynamic section from the loaded ELF
- Walk `RELA` entries, resolve symbol names from string table
- Look up each symbol in `LibKernel::has_symbol()` and write trampoline addresses into GOT
- Log unresolved symbols to `CompatTriage`

#### [MODIFY] [self_parser.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/loader/self_parser.cpp)
- Parse `PT_DYNAMIC` segment and extract `DT_NEEDED` library names + `DT_STRTAB`/`DT_SYMTAB` into `ParsedElf::needed_libraries` and `ParsedElf::imported_symbols`

---

### Group 4: Filesystem & Decompression (Phase 4)

Replace memcpy Kraken decoder with real LZ77 decompression; add VFS `lseek`.

---

#### [MODIFY] [kraken_decoder.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/decompression/kraken_decoder.cpp)
- Replace `memcpy` passthrough in the compressed branch with a clean-room LZ77 decoder (match offset + length decoding from bitstream)
- Keep raw passthrough for uncompressed flag

#### [MODIFY] [vfs.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/vfs.hpp)
- Add `int64_t seek_file(VfsFileHandle, int64_t offset, int whence)`

#### [MODIFY] [vfs.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/vfs.cpp)
- Implement `seek_file()` using stream seekg/seekp

---

### Group 5: GPU & Shader Recompilation (Phases 5-6)

Add real Vulkan initialization (VkInstance, VkDevice, real GPU detection); add RDNA2 instruction decoding to shader recompiler; add more GNM opcodes; add multi-threaded command buffer submission.

---

#### [MODIFY] [Dependencies.cmake](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/cmake/Dependencies.cmake)
- Add `find_package(Vulkan)` with optional/fallback behavior
- Add FetchContent for SPIRV-Headers (for SPIR-V constants)

#### [MODIFY] [vulkan_backend.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/vulkan_backend.hpp)
- Replace hardcoded `VulkanDeviceInfo` strings with real detected GPU info
- Add `VkInstance`, `VkPhysicalDevice`, `VkDevice` handles (behind `#ifdef QUIN_HAS_VULKAN`)
- Keep the simulation fallback when Vulkan SDK is not available

#### [MODIFY] [vulkan_backend.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/vulkan_backend.cpp)
- In `initialize()`: call `vkCreateInstance`, `vkEnumeratePhysicalDevices`, `vkGetPhysicalDeviceProperties`, `vkCreateDevice` to get real GPU name/VRAM/driver version
- Keep existing PSO cache logic but document that pipelines are tracked (not yet compiled into real Vulkan PSOs)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Add conditional Vulkan linking: `if(Vulkan_FOUND) target_compile_definitions(quin-core PUBLIC QUIN_HAS_VULKAN) target_link_libraries(quin-core PUBLIC Vulkan::Vulkan)`
- Add new source files (dynamic_linker.cpp, sce_np_trophy.cpp)

#### [MODIFY] [gnm_parser.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/gnm_parser.cpp)
- Add handlers for: `IT_NOP`, `IT_SET_BASE`, `IT_INDEX_TYPE`, `IT_DRAW_INDEX_2`, `IT_EVENT_WRITE`, `IT_WAIT_REG_MEM`
- Parse additional context registers (viewport, scissor, blend state)

#### [MODIFY] [shader_recompiler.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/shader_recompiler.hpp)
- Add `decode_rdna2_instruction()` private method
- Add RDNA2 opcode enum for recognized instructions

#### [MODIFY] [shader_recompiler.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/shader_recompiler.cpp)
- Decode RDNA2 instructions from the binary: `s_endpgm`, `v_mov_b32`, `v_add_f32`, `v_mul_f32`, `v_mad_f32`, `s_mov_b32`, `s_waitcnt`, `s_load_dwordx4`, `exp` (export instruction)
- Emit corresponding SPIR-V opcodes for each recognized RDNA2 instruction into the shader body (after the header)
- Fall through unrecognized instructions with a logged warning

---

### Group 6: Audio & Input (Phases 7-8)

Add real SDL2 audio device output; add real SDL2 controller polling.

---

#### [MODIFY] [audio_engine.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/audio/audio_engine.hpp)
- Add SDL audio device ID member (`SDL_AudioDeviceID`)
- Add `poll_audio()` method for mixing

#### [MODIFY] [audio_engine.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/audio/audio_engine.cpp)
- In `initialize()`: call `SDL_OpenAudioDevice()` with 48kHz stereo S16 spec
- In `submit_pcm_samples()`: call `SDL_QueueAudio()` to actually output PCM to speakers
- In `shutdown()`: call `SDL_CloseAudioDevice()`
- Add basic format conversion (F32→S16) and volume scaling

#### [MODIFY] [input_manager.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/input/input_manager.hpp)
- Add `SDL_GameController*` handles per pad
- Add `poll_input()` method to read real controller state

#### [MODIFY] [input_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/input/input_manager.cpp)
- In `initialize()`: scan for connected controllers via `SDL_NumJoysticks()` + `SDL_GameControllerOpen()`
- In `poll_input()`: read all axis/button values via `SDL_GameControllerGetAxis()`/`SDL_GameControllerGetButton()` and map to `PadState`
- In `open_pad()`: attempt to open a real SDL_GameController for the user_id index
- Add `SDL_GameControllerRumble()` call in `set_vibration()`
- Add `SDL_GameControllerSetLED()` call in `set_lightbar()`

#### [MODIFY] [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Call `m_input_manager.poll_input()` each frame in `render()` to get live controller data

---

### Group 7: Compatibility & Performance (Phases 9-10)

Real regression tests; connect triage to live syscall/symbol misses.

---

#### [MODIFY] [compat_triage.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/compat/compat_triage.cpp)
- `run_regression_suite()`: Actually test ELF loading, address space alloc/free, VFS mount/open/read/close, thread creation/join, syscall dispatch, shader cache put/get — report real pass/fail

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update roadmap table to show honest per-phase status (mix of ✅, ⚠️ Partial, 🔧 In Progress) instead of all "✅ Complete"
- Fix "SDL3" references → "SDL2"
- Fix claims about "Vulkan 1.3 Rendering Pipeline" to note it currently initializes Vulkan for GPU detection but does not yet render
- Fix "Kraken & Oodle Decompressor" to note LZ77 clean-room decoder (Oodle requires licensing)
- Update feature descriptions to be accurate about current implementation state

---

## Verification Plan

### Automated Tests
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

### Manual Verification
- Launch the debug shell and verify real GPU name appears in the GPU tab (from Vulkan)
- Connect a DualSense controller and verify button/stick input appears live in the Input tab
- Verify audio test tone plays through speakers when submitting PCM samples
- Load the homebrew ELF and step through more than just NOP/RET/SYSCALL instructions
- Check that the Linux CI build passes with signal handlers (no Windows-only gaps)
