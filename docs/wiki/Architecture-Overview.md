# 🚀 Architecture Overview — Quin PS5 Emulator

## 1. High-Level Subsystem Breakdown

Quin is structured as a modular translation layer designed to map PlayStation 5 console system services and hardware primitives directly to modern host operating systems (Windows, Linux, macOS).

```
+-------------------------------------------------------------------------------+
|                           Quin Host Workspace                                  |
|                                                                               |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  |  Executable Loader    | --> | Guest Address     | --> | Execution       |  |
|  |  (SELF / ELF Parser)  |     | Space (mmap/VEH)  |     | Engine (x86-64) |  |
|  +-----------------------+     +-------------------+     +-----------------+  |
|                                                                   |           |
|                                                                   v           |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  | FreeBSD / PS5         | <-- | System Modules    | <-- | Syscall         |  |
|  | Syscall Dispatcher    |     | (libSce* Stubs)   |     | Register Map    |  |
|  +-----------------------+     +-------------------+     +-----------------+  |
|             |                            |                        |           |
|             v                            v                        v           |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  | GNM GPU / RDNA2       |     | Tempest 3D Audio  |     | DualSense HID   |  |
|  | Shader Recompiler     |     | PCM Engine (SDL2) |     | Input Subsystem |  |
|  +-----------------------+     +-------------------+     +-----------------+  |
|             |                            |                        |           |
|             v                            v                        v           |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  | Vulkan 1.3 Backend    |     | Master 48kHz      |     | Normalized      |  |
|  | & Disk PSO Cache      |     | Audio Output      |     | PadState Maps   |  |
|  +-----------------------+     +-------------------+     +-----------------+  |
+-------------------------------------------------------------------------------+
```

---

## 2. Core Engine Components (`src/`)

### `src/loader/` — Executable Loader
- Parses 64-bit ELF and SELF (Signed ELF) header structures.
- Extracts `PT_LOAD` segments, virtual addresses (`p_vaddr`), memory sizes (`p_memsz`), and permission flags (`PF_R`, `PF_W`, `PF_X`).
- Bootstraps guest stack frames and transfers control to `e_entry`.

### `src/memory/` — Guest Memory Manager
- Allocates page-aligned guest virtual memory (`4 KB` page size) spanning PS5 user-space memory layouts (`0x0000000000400000ULL` to `0x00007FFFFFFFF000ULL`).
- Implements `mmap`, `munmap`, and `mprotect` memory protection flags (`PROT_READ`, `PROT_WRITE`, `PROT_EXEC`).
- Protects guest memory bounds with guard pages to catch null pointer dereferences.

### `src/cpu/` — Execution Harness & Threading
- Manages multi-threaded guest context execution via `ThreadManager`.
- Provides per-thread Thread-Local Storage (TLS) allocation and stack frame boundaries.
- Intercepts native access violations using Windows Vectored Exception Handlers (VEH) and POSIX signals without crashing the host process.

### `src/kernel/` — Syscall Dispatcher & `libSce*` Modules
- Dispatches FreeBSD/PS5 syscalls via standard System V AMD64 ABI register conventions (`RAX`, `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`).
- Registers system libraries (`libkernel`, `libSceLibcInternal`, `libSceSystemService`, `libSceUserService`, `libSceAudioOut`, `libScePad`).

### `src/gpu/` — GPU Processing & Vulkan Backend
- Parses AMD GNM PM4 Type-3 command packets (`IT_DRAW_INDEX_AUTO`, `IT_SET_CONTEXT_REG`).
- Translates GNM surface formats (`R8G8B8A8_UNORM`, `R32_SFLOAT`) to `VkFormat` with state-hash-keyed Pipeline State Object (PSO) caching.
- Features `PsoDiskCache` for serializing compiled Vulkan PSOs to disk files (`.quin_pso_cache`).

### `src/gpu/shader/` — RDNA2 Shader Recompiler
- Decodes RDNA2 ISA bytecode instructions and emits standard SPIR-V 1.5 binary modules (`OpEntryPoint`, `OpCapability Shader`, `OpMemoryModel Logical GLSL450`).
- Features `AsyncShaderCompiler` multi-threaded worker pool compiling shaders off the render thread.

### `src/audio/` — Tempest 3D Audio Engine
- Routes 48 kHz multi-channel PCM audio streams through low-latency SDL2 audio device ring buffers.
- Implements volume panning, channel routing, and `libSceAudioOut` symbol stubs.

### `src/input/` — Input Subsystem & DualSense Driver
- Communicates directly with PS5 DualSense controllers over USB/Bluetooth HID reports and host SDL2 gamepads.
- Normalizes button bitmasks, analog stick coordinates (`-128` to `+127`), RGB lightbar colors, and vibration feedback.

### `src/fs/` — Virtual Filesystem & Storage
- Maps guest virtual paths (`/app0/`, `/data/`, `/system/`, `/savedata/`) to host disk directories.
- Implements Kraken & Oodle chunked byte-stream decompression pipelines.
- Manages isolated savedata containers per user ID and title ID.

### `src/compat/` — Compatibility Database & Triage
- Maintains per-title status database (`Boots`, `Menu`, `Ingame`, `Playable`, `Perfect`).
- Records missing system symbol call counts and provides an automated regression suite runner.
