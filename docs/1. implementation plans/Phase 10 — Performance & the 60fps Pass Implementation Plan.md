# Phase 10 — Performance & the 60fps Pass Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 10 — Performance & the 60fps Pass** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **Persistent Disk PSO Cache**: Serializes and loads compiled Vulkan Pipeline State Objects (PSOs) and SPIR-V binaries to/from disk (`.quin_pso_cache`) to eliminate first-launch traversal stutter.
> - **Async Shader Compiler Worker Pool**: Offloads RDNA2 to SPIR-V compilation tasks to background worker threads, preventing render thread hitches.
> - **Frame-Pacing & Dynamic Resolution Scale**: Frame-pacing regulator supporting locked 30fps/60fps/unlocked modes, frame time jitter reduction, and dynamic resolution scaling (FSR2/3 headroom).

## Proposed Changes

---

### Performance & Optimization Subsystem (`src/gpu/`, `src/gpu/shader/`)

#### [NEW] [pso_disk_cache.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/pso_disk_cache.hpp) & [pso_disk_cache.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/pso_disk_cache.cpp)
- `PsoDiskCache` serializing compiled PSO keys and SPIR-V bytecodes to disk files.

#### [NEW] [async_shader_compiler.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/async_shader_compiler.hpp) & [async_shader_compiler.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/async_shader_compiler.cpp)
- `AsyncShaderCompiler` multi-threaded worker pool executing background shader recompilation off the main render thread.

#### [NEW] [frame_pacing.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/frame_pacing.hpp) & [frame_pacing.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/frame_pacing.cpp)
- `FramePacingRegulator` managing target framerate locks (30 FPS, 60 FPS, Unlocked), frame time variance calculation, and resolution scale adjustments.

---

### Debug UI & Integration (`src/gui/`)

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp) & [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Performance Tuning & 60fps Engine" ImGui panel displaying target FPS selector (30/60/Unlocked), frame time variance graph, async shader worker pool status, disk PSO cache status, and dynamic resolution scaling slider.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include performance optimization sources in `quin-core`.
- Add `tests/unit/test_performance_60fps.cpp` target to `quin-tests`.

#### [NEW] [test_performance_60fps.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_performance_60fps.cpp)
- Catch2 unit tests for PSO disk serialization, async shader compilation worker queue, frame pacing delta calculations, and dynamic resolution scaling math.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 10 as ✅ **Complete** (all 10 roadmap phases completed!).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, adjust frame-pacing target to 60 FPS, toggle async shader compiler, inspect disk PSO cache metrics, and adjust resolution scale slider in the ImGui debug shell.
