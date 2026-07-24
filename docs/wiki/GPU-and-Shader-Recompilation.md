# 🎮 GPU & Shader Recompilation — Quin PS5 Emulator

## 1. GNM PM4 Command Processing (`src/gpu/`)

### GNM PM4 Packet Parser (`gnm_parser.hpp`/`cpp`)
- `GnmCmdParser` parses AMD GNM PM4 Type-3 command packets submitted by guest render threads.
- Extracts draw commands, index counts, vertex offsets, and primitive topologies.

#### Supported PM4 Packets
- `IT_DRAW_INDEX_AUTO` (`0x2D`): Draw indexed primitive auto-increment call.
- `IT_SET_CONTEXT_REG` (`0xA4`): Updates context registers (render target format, depth format, blend mode).
- `IT_INDEX_TYPE` (`0x2A`): Sets index buffer element size (16-bit / 32-bit uint).

### Resource & Format Translator (`resource_translator.hpp`/`cpp`)
- `GnmToVulkanTranslator` maps GNM surface formats and topologies to native Vulkan 1.3 equivalents:
  - `GnmSurfaceFormat::R8G8B8A8_UNORM` ➔ `VK_FORMAT_R8G8B8A8_UNORM`
  - `GnmSurfaceFormat::B8G8R8A8_UNORM` ➔ `VK_FORMAT_B8G8R8A8_UNORM`
  - `GnmSurfaceFormat::R32_SFLOAT` ➔ `VK_FORMAT_R32_SFLOAT`
  - `GnmPrimitiveType::TriangleList` ➔ `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`
  - `GnmPrimitiveType::TriangleStrip` ➔ `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP`

### Vulkan 1.3 Graphics Backend (`vulkan_backend.hpp`/`cpp`)
- `VulkanBackend` initializes Vulkan 1.3 physical device, logical device, graphics queue, and command pools.
- Implements state-hash-keyed Pipeline State Object (PSO) caching (`cached_pipelines_count`).

---

## 2. RDNA2 Shader Recompiler (`src/gpu/shader/`)

### Shader Recompiler Engine (`shader_recompiler.hpp`/`cpp`)
- `ShaderRecompiler` decodes AMD RDNA2 ISA bytecode instructions and emits standard SPIR-V 1.5 binary modules (`OpEntryPoint`, `OpCapability Shader`, `OpMemoryModel Logical GLSL450`).
- Supports **Vertex Shader** (`ShaderType::Vertex`) and **Pixel/Fragment Shader** (`ShaderType::Pixel`) stages.

### Persistent Binary Shader Cache (`shader_cache.hpp`/`cpp`)
- `ShaderCache` generates 64-bit FNV-1a hash keys (`ShaderHash`) over RDNA2 bytecode.
- Maintains in-memory lookup cache to eliminate redundant recompilation hits.
- Persists compiled SPIR-V modules to disk.

### Async Shader Compiler Pool (`async_shader_compiler.hpp`/`cpp`)
- `AsyncShaderCompiler` offloads RDNA2 to SPIR-V recompilation tasks to a dedicated worker thread pool (`m_worker_threads`).
- Prevents render thread hitches and frame stutters during game traversal.
