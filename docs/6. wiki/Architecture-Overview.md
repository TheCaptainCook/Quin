# 🚀 Architecture Overview — Quin PS5 Emulator

## 1. High-Level Subsystem Breakdown

Quin is structured as a modular translation layer designed to map PlayStation 5 console system services and hardware primitives directly to modern host operating systems (Windows, Linux, macOS).

```
+-------------------------------------------------------------------------------+
|                           Quin Host Workspace                                  |
|                                                                               |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  |  Executable Loader    | --> | Guest Address     | --> | Execution       |  |
|  |  (SELF/ELF + DynLink) |     | Space (mmap/VEH)  |     | Engine (~20 ops)|  |
|  +-----------------------+     +-------------------+     +-----------------+  |
|                                                                   |           |
|                                                                   v           |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  | FreeBSD / PS5 (23)    | <-- | System Modules    | <-- | Syscall         |  |
|  | Syscall Dispatcher    |     | (libSce* Stubs)   |     | Register Map    |  |
|  +-----------------------+     +-------------------+     +-----------------+  |
|             |                            |                        |           |
|             v                            v                        v           |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  | GNM GPU / RDNA2 (~40) |     | Tempest 3D Audio  |     | DualSense HID   |  |
|  | Shader Recompiler     |     | PCM Engine (SDL2) |     | Input Subsystem |  |
|  +-----------------------+     +-------------------+     +-----------------+  |
|             |                            |                        |           |
|             v                            v                        v           |
|  +-----------------------+     +-------------------+     +-----------------+  |
|  | Vulkan Backend        |     | Master 48kHz      |     | Normalized      |  |
|  | (Hardware Detection)  |     | Audio Output      |     | PadState Maps   |  |
|  +-----------------------+     +-------------------+     +-----------------+  |
+-------------------------------------------------------------------------------+
```

---

## 2. Core Engine Components (`src/`)

### `src/loader/` — Executable Loader & Dynamic Linker
- Parses 64-bit ELF and SELF (Signed ELF) header structures.
- Extracts `PT_LOAD` segments, virtual addresses (`p_vaddr`), memory sizes (`p_memsz`), and permission flags (`PF_R`, `PF_W`, `PF_X`).
- `DynamicLinker` parses `PT_DYNAMIC` segments, walks `RELA` / `JMPREL` relocation tables, extracts `DT_NEEDED` library names, resolves symbols against `LibKernel` stubs, and patches GOT trampoline entries.

### `src/memory/` — Guest Memory Manager
- Allocates page-aligned guest virtual memory (`4 KB` page size) spanning PS5 user-space memory layouts (`0x0000000000400000ULL` to `0x00007FFFFFFFF000ULL`).
- Implements `mmap`, `munmap`, and `mprotect` memory protection flags (`PagePermission::Read`, `Write`, `Execute`, `ReadWriteExecute`).
- Protects guest memory bounds with guard pages to catch null pointer dereferences.

### `src/cpu/` — Execution Harness & Threading
- Manages multi-threaded guest context execution via `ThreadManager` with instruction stepping on spawned threads.
- Provides per-thread Thread-Local Storage (TLS) allocation with self-pointer and thread ID layout, including `arch_prctl(ARCH_SET_FS)` on Linux x86-64.
- `ExecutionEngine` includes an x86-64 instruction decoder handling ~20 opcodes (`MOV`, `PUSH`, `POP`, `CALL`, `JMP`, `Jcc`, `ADD`, `SUB`, `XOR`, `CMP`, `TEST`, `LEA`, `LEAVE`, `INT3`, `HLT`, `SYSCALL`).
- Intercepts native access violations using Windows Vectored Exception Handlers (VEH) and POSIX signal handlers (`SIGSEGV`, `SIGILL`, `SIGBUS`, `SIGTRAP`) with `ucontext_t` register dumps.

### `src/kernel/` — Syscall Dispatcher & `libSce*` Modules
- Dispatches 23 FreeBSD/PS5 syscalls via standard System V AMD64 ABI register conventions (`RAX`, `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`).
- Registers system libraries (`libkernel`, `libSceLibcInternal`, `libSceSystemService`, `libSceUserService`, `libSceAudioOut`, `libScePad`, `libSceNpTrophy`).

### `src/gpu/` — GPU Processing & Vulkan Backend
- Parses 8 AMD GNM PM4 Type-3 command packet types (`IT_NOP`, `IT_SET_BASE`, `IT_INDEX_TYPE`, `IT_DRAW_INDEX_AUTO`, `IT_DRAW_INDEX_2`, `IT_SET_CONTEXT_REG`, `IT_EVENT_WRITE`, `IT_WAIT_REG_MEM`).
- Features real Vulkan hardware GPU detection via `VkInstance` and `VkPhysicalDevice` properties (device name, driver version, VRAM size) with fallback to simulated mode.
- Caches Pipeline State Objects (PSO) with disk serialization (`.quin_pso_cache`).

### `src/gpu/shader/` — RDNA2 Shader Recompiler
- Decodes RDNA2 ISA bytecode instructions (~40 instructions across SOPP, SOP1, SOP2, SOPC, VOP1, VOP2, VOP3, SMEM, EXP) and emits standard SPIR-V 1.5 binary modules.
- Features `AsyncShaderCompiler` multi-threaded worker pool compiling shaders off the render thread.

### `src/audio/` — Tempest 3D Audio Engine
- Routes 48 kHz multi-channel PCM audio streams through real host SDL2 audio devices via `SDL_OpenAudioDevice` and `SDL_QueueAudio`.
- Implements volume scaling, F32→S16 format conversion, and `libSceAudioOut` symbol stubs.

### `src/input/` — Input Subsystem & DualSense Driver
- Communicates directly with PS5 DualSense controllers and host gamepads via SDL2 GameController API.
- Normalizes button bitmasks, analog stick coordinates (`-128` to `+127`), RGB lightbar colors, rumble haptics, and hotplug events.

### `src/fs/` — Virtual Filesystem & Storage
- Maps guest virtual paths (`/app0/`, `/data/`, `/system/`, `/savedata/`) to host disk directories with `seek_file` support.
- Implements a clean-room LZ77 decompression algorithm for KRAK-magic compressed asset chunks.
- Manages isolated savedata containers per user ID and title ID.

### `src/compat/` — Compatibility Database & Triage
- Maintains per-title status database (`Boots`, `Menu`, `Ingame`, `Playable`, `Perfect`).
- Records missing system symbol call counts fed by the dynamic linker and provides a 6-case real functional regression suite.
