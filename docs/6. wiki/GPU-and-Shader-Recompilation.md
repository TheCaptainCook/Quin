# 🎮 GPU & Shader Recompilation — Quin PS5 Emulator

## 1. GNM GPU Command Buffer Processing (`src/gpu/`)

### GNM PM4 Type-3 Packet Parser (`gnm_parser.hpp`/`cpp`)
`GnmCmdParser` intercepts AMD GNM graphics command ring buffers in guest memory, decoding 8 PM4 Type-3 packet types:
- `IT_NOP` (`0x10`): No-operation padding packet.
- `IT_SET_BASE` (`0x11`): Sets indirect buffer base address.
- `IT_INDEX_TYPE` (`0x2A`): Sets index buffer element type (16-bit / 32-bit).
- `IT_DRAW_INDEX_AUTO` (`0x2D`): Non-indexed draw command.
- `IT_DRAW_INDEX_2` (`0x36`): Indexed draw command with index buffer address and max index count.
- `IT_SET_CONTEXT_REG` (`0x69`): Updates render context registers:
  - `0x01`: Color Render Target format (`R8G8B8A8_UNORM`, `B8G8R8A8_UNORM`, etc.)
  - `0x02`: Depth/Stencil Target format (`D32_SFLOAT`, etc.)
  - `0x10`: `CB_BLEND_CONTROL` blend enable state
  - `0x20`: `DB_DEPTH_CONTROL` depth test & depth write enable state
  - `0x30`: `PA_SC_VPORT_SCISSOR` viewport dimensions
- `IT_EVENT_WRITE` (`0x46`): Flushes pipeline caches / event triggers.
- `IT_WAIT_REG_MEM` (`0x3C`): Waits for register/memory condition.

### Vulkan Hardware Backend (`vulkan_backend.hpp`/`cpp`)
- Performs real Vulkan hardware initialization when Vulkan SDK is present (`QUIN_HAS_VULKAN`):
  - Creates `VkInstance`
  - Enumerates physical devices and selects primary GPU
  - Reads `VkPhysicalDeviceProperties` (GPU device name, driver version, Vulkan API version)
  - Reads `VkPhysicalDeviceMemoryProperties` for total device-local VRAM size
  - Creates logical `VkDevice` with graphics queue
- Falls back gracefully to simulated mode when Vulkan SDK is omitted.
- Manages Pipeline State Object (PSO) caching keyed by `PsoKey` state hashes.

---

## 2. RDNA2 Shader Recompiler (`src/gpu/shader/`)

### Clean-Room RDNA2 Instruction Decoder (`shader_recompiler.hpp`/`cpp`)
`ShaderRecompiler` parses AMD RDNA2 GPU bytecode instructions, categorizing top dword bits across 8 ISA categories:
1. **SOPP** (Scalar Program Flow): `s_nop`, `s_endpgm`, `s_branch`, `s_cbranch_*`, `s_barrier`, `s_waitcnt`.
2. **SOP1** (Scalar One-Operand): `s_mov_b32`, `s_mov_b64`, `s_getpc_b64`, `s_swappc_b64`.
3. **SOP2** (Scalar Two-Operand): `s_add_u32`, `s_sub_u32`, `s_and_b32`, `s_or_b32`, `s_xor_b32`, `s_lshl_b32`, `s_lshr_b32`.
4. **SOPC** (Scalar Comparison): `s_cmp_*`.
5. **VOP1** (Vector One-Operand): `v_mov_b32`, `v_cvt_f32_i32`, `v_cvt_i32_f32`, `v_rcp_f32`, `v_rsq_f32`, `v_sqrt_f32`.
6. **VOP2** (Vector Two-Operand): `v_add_f32`, `v_sub_f32`, `v_mul_f32`, `v_mac_f32`, `v_max_f32`, `v_min_f32`, `v_add_u32`, `v_sub_u32`, `v_and_b32`, `v_or_b32`, `v_xor_b32`.
7. **VOP3** (Vector Three-Operand): 64-bit vector instructions.
8. **SMEM** (Scalar Memory): `s_load_dword`, `s_load_dwordx2`, `s_load_dwordx4`, `s_load_dwordx8`.
9. **EXP** (Export): Fragment/position export instructions.

Emits valid SPIR-V 1.5 binary modules with header, capabilities, execution model, type declarations, function body, and OpReturn/OpFunctionEnd.

### Persistent Shader Caching & Async Compilation
- `ShaderCache`: In-memory shader cache keyed by 64-bit binary content hash.
- `AsyncShaderCompiler`: Dedicated worker thread queue compiling shaders off the render thread.
- `PsoDiskCache`: Serializes compiled PSO configurations to `.quin_pso_cache`.
