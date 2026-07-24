# ⚡ Performance & Optimization — Quin PS5 Emulator

## 1. Persistent Disk PSO Cache (`src/gpu/pso_disk_cache.hpp`/`cpp`)

- **Serialization Engine**: `PsoDiskCache` serializes compiled Vulkan Pipeline State Objects (PSOs) and SPIR-V bytecodes to disk cache files (`.quin_pso_cache`).
- **Binary Header Validation**: Validates file magic header `0x5150534F` (`QPSO`) on startup.
- **Traversal Stutter Elimination**: Loads pre-compiled PSOs into Vulkan memory on application launch, eliminating first-time rendering hitches.

---

## 2. Async Shader Compilation Worker Pool (`src/gpu/shader/async_shader_compiler.hpp`/`cpp`)

- **Multi-Threaded Queue**: `AsyncShaderCompiler` offloads RDNA2 to SPIR-V recompilation tasks to a dedicated worker thread pool (`m_worker_threads`).
- **Non-Blocking Render Loop**: If a shader is not yet compiled, the render engine uses a fallback pipeline while background workers finish recompiling the target shader.
- **Thread Safety**: Job queue synchronized with mutexes (`m_queue_mutex`) and condition variables (`m_cv`).

---

## 3. Frame-Pacing Regulator & Dynamic Resolution Scaling (`src/gpu/frame_pacing.hpp`/`cpp`)

- **Frame Pacing Modes**:
  - `Locked60`: Target frame time `16.666 ms` (60 FPS lock).
  - `Locked30`: Target frame time `33.333 ms` (30 FPS lock).
  - `Unlocked`: Unlimited framerate mode.
- **Frame Delta & Variance Tracking**: Calculates high-resolution frame microsecond deltas using `std::chrono::high_resolution_clock`.
- **Dynamic Resolution Scaling (FSR)**: Adjusts rendering resolution scale factor dynamically (`0.5x` to `1.0x`) for upscaling performance headroom.
