# 🎮 Quin — PlayStation 5 Console Emulator & Translation Layer

> A high-performance, open-source PlayStation 5 (PS5) console emulator and x86-64 translation layer built with modern C++20, Vulkan 1.3 graphics, Tempest 3D Audio, and a visual Debug Shell UI.

---

## 💡 What is Quin? (In Plain English)

**Quin** is a software translation layer that allows computers (Windows, Linux, and macOS) to run PlayStation 5 software. 

Think of Quin as a universal translator: PS5 games speak a specific "language" designed for console hardware. Quin intercepts those instructions in real time and translates them into a language your PC’s CPU and graphics card (NVIDIA, AMD, or Intel) can understand.

### Key Highlights for Everyday Users
- **🎮 Plug & Play DualSense Support**: Full native support for PS5 DualSense controllers, including RGB lightbar feedback, analog sticks, and rumble vibration.
- **⚡ Smooth 60 FPS Engine**: Includes automatic frame-pacing, persistent shader caching (no stuttering on startup), and dynamic resolution scaling (FSR).
- **🔊 Tempest 3D Audio**: Recreates spatial PS5 console sound through standard 48 kHz headphones or PC speakers.
- **🖥️ Built-in Visual Control Panel**: An interactive debug shell allows you to load software, view performance graphs, track game compatibility, and inspect memory in real time.
- **🔒 100% Clean-Room & Legal**: Built entirely from scratch using open-source FreeBSD standards and public graphics hardware manuals — zero proprietary console software required.

---

## ⚙️ How It Works Under the Hood

```
[ PS5 Game / Homebrew ELF ] 
          │
          ▼
   [ Executable Loader ] ────► Parses 64-bit ELF headers & maps guest virtual memory
          │
          ▼
   [ CPU Execution Engine ] ──► Translates x86-64 code & dispatches FreeBSD / PS5 Syscalls
          │
   ┌──────┴──────────────────────────────────────┐
   ▼                                             ▼
[ GNM GPU & RDNA2 Shader ]              [ Tempest 3D Audio & Input ]
   │                                             │
   ▼                                             ▼
[ Vulkan 1.3 Render Engine ]             [ SDL2 Audio & DualSense HID ]
```

1. **Executable Loading**: Reads 64-bit PS5 executable files (`.elf` and `.self`) and securely maps them into guest memory.
2. **CPU Execution**: Runs multi-threaded game logic across modern CPU cores with thread-local storage (TLS) isolation.
3. **Graphics Translation**: Converts PS5 GNM GPU commands and RDNA2 shader bytecode directly into standard **Vulkan 1.3** SPIR-V instructions.
4. **Audio & Controls**: Routes multi-channel PCM audio streams to your speakers and maps DualSense controller inputs instantly.

---

## 🚀 Detailed System Features

### 1. 🖥️ Graphics & Shader Translation (`src/gpu/`)
- **Vulkan 1.3 Rendering Pipeline**: Translates PS5 GNM surface formats (`R8G8B8A8_UNORM`, `B8G8R8A8_UNORM`, `R32_SFLOAT`) to native Vulkan formats with state-hash-keyed Pipeline State Object (PSO) caching.
- **RDNA2 ISA Shader Recompiler**: Clean-room instruction decoder that converts RDNA2 GPU bytecode into valid SPIR-V 1.5 modules for Vertex and Pixel (Fragment) shader stages.
- **Persistent Disk PSO Cache**: Saves compiled graphics pipelines to disk (`.quin_pso_cache`) to eliminate first-launch stuttering.
- **Async Shader Compilation**: Compiles shaders in the background using dedicated worker threads off the main render loop.

### 2. 🔊 Audio & Input Subsystem (`src/audio/`, `src/input/`)
- **Tempest 3D Audio Engine**: Processes multi-channel 48 kHz PCM sample streams with volume panning, channel routing, and SDL2 device output.
- **DualSense Controller Driver**: Directly communicates with PS5 DualSense controllers over USB and Bluetooth, supporting button bitmasks, analog stick axes, RGB lightbar colors, and vibration feedback.

### 3. 💾 Filesystem & Storage (`src/fs/`)
- **Virtual Filesystem (VFS)**: Maps PS5 virtual directories (`/app0/`, `/data/`, `/system/`, `/savedata/`) to host folder paths with full POSIX file access support (`SYS_open`, `SYS_read`, `SYS_write`, `SYS_close`).
- **Kraken & Oodle Decompressor**: Decodes compressed PS5 game asset chunks on the fly.
- **SaveData Manager**: Isolated savedata container management per user ID and game title (`/savedata/<user_id>/<title_id>/`).

### 4. 🧠 Memory & System Libraries (`src/memory/`, `src/kernel/`)
- **Dynamic Memory Manager**: Page-aligned `mmap`, `munmap`, and `mprotect` guest memory management matching PS5 64-bit virtual memory address layouts.
- **FreeBSD / PS5 Syscall Architecture**: Syscall dispatcher handling standard system calls with full x86-64 ABI register mapping (`RAX`, `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`).
- **`libSce*` System Library Stubs**: Implements module stubs for `libSceLibcInternal`, `libSceSystemService`, `libSceUserService`, `libSceAudioOut`, and `libScePad`.

### 5. 📊 Compatibility & Performance Engine (`src/compat/`)
- **Per-Title Status Tracker**: Built-in title database tracking compatibility ratings (`Boots`, `Menu`, `In-game`, `Playable`, `Perfect`).
- **Automated Symbol Triage Logger**: Automatically logs missing system functions by frequency so developers know exactly what to implement next.
- **Frame-Pacing Regulator**: Built-in 30 FPS, 60 FPS, and Unlocked frame pacing controls with dynamic resolution scaling (FSR2/3 headroom).

---

## 📜 Development Roadmap Progress

| Phase | Milestone | Focus Area | Status |
| :--- | :--- | :--- | :---: |
| **Phase 0** | **Foundations & Tooling** | C++20 build system, ImGui visual debug shell, spdlog console | ✅ **Complete** |
| **Phase 1** | **Executable Loading** | 64-bit SELF/ELF parser, guest address space manager, libkernel stubs | ✅ **Complete** |
| **Phase 2** | **CPU & Memory Model** | Multi-threaded thread manager, TLS isolation, native exception handling | ✅ **Complete** |
| **Phase 3** | **Syscalls & System Modules** | FreeBSD syscall dispatcher, `libSceLibcInternal`, `libSceSystemService` | ✅ **Complete** |
| **Phase 4** | **Filesystem & Storage** | VFS mount table, SaveData manager, Kraken/Oodle decompression | ✅ **Complete** |
| **Phase 5** | **GPU & Vulkan Backend** | GNM PM4 packet parser, Vulkan 1.3 backend, state-hash PSO cache | ✅ **Complete** |
| **Phase 6** | **Shader Recompilation** | Clean-room RDNA2 ISA decoder, SPIR-V bytecode emitter, shader cache | ✅ **Complete** |
| **Phase 7** | **Audio Subsystem** | Tempest 3D Audio engine, 48 kHz multi-channel PCM output | ✅ **Complete** |
| **Phase 8** | **Input Subsystem** | DualSense HID controller driver, `libScePad` module stubs | ✅ **Complete** |
| **Phase 9** | **Compatibility Expansion** | Per-title database, stub triage logger, regression test runner | ✅ **Complete** |
| **Phase 10** | **Performance & 60 FPS** | Persistent disk PSO cache, async shader compiler, frame pacing | ✅ **Complete** |

Compatibility matrix available in [`compatibility.md`](compatibility.md).

---

## 🛠️ Building & Running (Quick Start)

### System Requirements
- **Operating System**: Windows 10/11 (64-bit), Linux (Ubuntu 22.04+), or macOS (12+)
- **Compiler**: C++20 compliant compiler (MSVC 2022, GCC 12+, or Clang 15+)
- **Build Tool**: CMake 3.22 or higher
- **Graphics Card**: Vulkan 1.3 compatible GPU (NVIDIA GTX 10-series or newer, AMD Radeon RX 500-series or newer, Intel Arc)

### Step-by-Step Build Instructions

```bash
# 1. Clone the repository
git clone https://github.com/TheCaptainCook/Quin.git
cd Quin

# 2. Configure the build with CMake (automatically downloads required dependencies)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile the executable and test suite
cmake --build build --config Release

# 4. Run automated unit verification tests
ctest --test-dir build -C Release --output-on-failure

# 5. Launch the Quin Debug Shell application
./build/bin/Release/quin.exe
```

---

## ❓ Frequently Asked Questions (FAQ)

#### Q: Can Quin run commercial PS5 games?
**A:** Quin is an open-source emulator project actively expanding compatibility. It currently runs homebrew software, technical test demos, and 2D/3D benchmarks while continuously improving title compatibility.

#### Q: Do I need a real PS5 console or proprietary firmware files?
**A:** No! Quin is developed strictly using **clean-room engineering**. It does not require proprietary Sony firmware or copyrighted console files to run homebrew binaries.

#### Q: Can I use a controller other than DualSense?
**A:** Yes! Quin includes full fallback support for standard PC gamepads (Xbox controllers, DualShock 4) and keyboard/mouse input via SDL2.

---

## 📄 Licensing & Terms of Use

This project is licensed under the **[Non-Commercial Personal Use License](LICENSE)**:
- **Personal & Educational Use**: Free for personal, non-commercial, academic, and research purposes.
- **No Commercial Use**: Any commercial, for-profit, or revenue-generating use is strictly prohibited.
- **Attribution Required**: Proper credit to the original author (**Quin Project by Masem**) must be included in all copies or derivative works.
