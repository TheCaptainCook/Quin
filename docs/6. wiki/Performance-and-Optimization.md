# ⚡ Performance & Optimization — Quin PS5 Emulator

## 1. Frame Pacing Regulator (`src/gpu/frame_pacing.hpp`/`cpp`)

`FramePacingRegulator` manages frame timing and target frame rates:
- **Target Modes**: `FPS_30`, `FPS_60`, `FPS_Unlocked`.
- **Dynamic Resolution Scale**: Resolution scaling factor (`0.5x` to `2.0x`) for rendering output.
- **Frame Timing**: Measures frame time delta, calculates CPU/GPU execution time, and throttles frame presentation using high-resolution sleep to maintain smooth frame pacing.

---

## 2. Persistent Disk PSO Cache (`src/gpu/pso_disk_cache.hpp`/`cpp`)

`PsoDiskCache` avoids runtime pipeline compilation stuttering:
- Serializes state-hash-keyed Pipeline State Objects (PSO) to binary cache files (`.quin_pso_cache`).
- Reads binary cache on startup, pre-populating `VulkanBackend` PSO lookup tables.

---

## 3. Async Shader Compilation Pool (`src/gpu/shader/async_shader_compiler.hpp`/`cpp`)

`AsyncShaderCompiler` offloads shader recompilation from the main render thread:
- Spawns multi-threaded worker threads.
- Accepts shader compilation jobs asynchronously via thread-safe queue.
- Places compiled SPIR-V modules into `ShaderCache` upon completion.
