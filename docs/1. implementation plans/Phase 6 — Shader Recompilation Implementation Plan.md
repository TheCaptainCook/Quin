# Phase 6 — Shader Recompilation Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 6 — Shader Recompilation** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **RDNA2 ISA to SPIR-V Translation**: Decodes PS5 RDNA2 shader binaries and generates valid SPIR-V bytecode modules (`OpEntryPoint`, `OpCapability Shader`, `OpMemoryModel Logical GLSL450`) for Vulkan pipeline creation.
> - **Binary Shader Cache**: Hashes RDNA2 shader bytecode to cache compiled SPIR-V binaries across launches, eliminating shader compilation stutter.
> - **Shader Stages**: Focuses on Vertex and Pixel (Fragment) shaders, with extensibility for Compute shaders.

## Proposed Changes

---

### Shader Recompiler Subsystem (`src/gpu/shader/`)

#### [NEW] [shader_types.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/shader_types.hpp)
- `ShaderType` (`Vertex`, `Pixel`, `Compute`), `ShaderHash`, `CompiledShader`, and SPIR-V magic header constants (`0x07230203`).

#### [NEW] [shader_recompiler.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/shader_recompiler.hpp) & [shader_recompiler.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/shader_recompiler.cpp)
- `ShaderRecompiler` decoding RDNA2 instructions (`V_ADD_F32`, `V_MUL_F32`, `EXP`, `S_ENDPGM`) and emitting binary SPIR-V bytecode instructions.

#### [NEW] [shader_cache.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/shader_cache.hpp) & [shader_cache.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gpu/shader/shader_cache.cpp)
- `ShaderCache` storing compiled SPIR-V bytecode keyed by 64-bit RDNA2 shader binary hash.

---

### Debug UI & Integration (`src/gui/`)

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp) & [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Shader Recompiler & SPIR-V Cache" ImGui panel displaying compiled shader counts per stage, cache hits, and SPIR-V bytecode inspector.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include shader recompiler sources in `quin-core`.
- Add `tests/unit/test_shader_recompiler.cpp` target to `quin-tests`.

#### [NEW] [test_shader_recompiler.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_shader_recompiler.cpp)
- Catch2 unit tests for RDNA2 shader binary parsing, SPIR-V header generation, vertex/pixel shader translation, and shader cache lookup.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 6 as ✅ **Complete** and Phase 7 as 🟡 **Next** (before git push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, inspect compiled vertex/pixel shaders and SPIR-V cache hits in the ImGui debug shell.
