# Phase 5 — GPU Command Processing → Vulkan Translation Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 5 — GPU Command Processing → Vulkan Translation** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **GNM PM4 Command Buffer Parser**: Parses PM4 Type-3 packet headers (`IT_DRAW_INDEX_AUTO`, `IT_SET_CONTEXT_REG`, `IT_NUM_INSTANCES`) from guest GPU memory buffers.
> - **Vulkan Pipeline State Caching (PSO)**: Caches Vulkan pipeline state objects by hashing GNM render state descriptors to avoid frame drops from on-the-fly pipeline compilation.
> - **Resource Format Translation**: Translates GNM surface/texture formats (`R8G8B8A8_UNORM`, `B8G8R8A8_UNORM`, `R32_SFLOAT`) to `VkFormat`.

## Proposed Changes

---

### GPU & Command Buffer Subsystem (`src/gpu/`)

#### [NEW] [gnm_types.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/gnm_types.hpp)
- Define GNM PM4 packet headers, opcode constants (`IT_DRAW_INDEX_AUTO`, `IT_SET_CONTEXT_REG`), primitive topologies, and render target formats.

#### [NEW] [gnm_parser.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/gnm_parser.hpp) & [gnm_parser.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/gnm_parser.cpp)
- `GnmCmdParser` ring buffer parser reading PM4 packets from guest memory and generating normalized `GnmDrawCommand` structures.

---

### Vulkan Backend & Resource Translator (`src/gpu/`)

#### [NEW] [resource_translator.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/resource_translator.hpp) & [resource_translator.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/resource_translator.cpp)
- Map GNM surface formats to `VkFormat` and primitive types (`Triangles`, `TriangleStrip`, `Lines`) to `VkPrimitiveTopology`.

#### [NEW] [vulkan_backend.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/vulkan_backend.hpp) & [vulkan_backend.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/vulkan_backend.cpp)
- `VulkanBackend` managing device initialization, Vulkan pipeline state caching (`PsoCache`), render passes, and queue submissions.

---

### Debug UI & Integration (`src/gui/`)

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp) & [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "GPU & Vulkan Translation" ImGui panel rendering GPU telemetry, PM4 packet metrics, PSO cache hits, and draw call counter.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include GPU sources in `quin-core`.
- Add `tests/unit/test_gpu_vulkan.cpp` target to `quin-tests`.

#### [NEW] [test_gpu_vulkan.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_gpu_vulkan.cpp)
- Catch2 unit tests for GNM PM4 packet parsing, GNM to `VkFormat` translation, PSO state hash generation, and Vulkan backend state.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 5 as ✅ **Complete** and Phase 6 as 🟡 **Next** (before git push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, view GPU command parser stats, Vulkan backend state, and PSO cache metrics in the ImGui debug shell.
