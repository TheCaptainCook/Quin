# 📖 Quin PS5 Emulator — Official Project Wiki

Welcome to the official technical wiki for **Quin**, a lean, high-performance PlayStation 5 (PS5) console emulator and x86-64 translation layer built with modern C++20, Vulkan 1.3, Tempest 3D Audio, and a responsive ImGui Debug Shell UI.

---

## 📚 Wiki Table of Contents

| Section | Description |
| :--- | :--- |
| 🚀 **[Architecture Overview](Architecture-Overview)** | System design, subsystem interaction, dataflow pipelines, and clean-room policy |
| 📜 **[Roadmap & Status](Roadmap-and-Status)** | Detailed 10-phase development roadmap, exit criteria, and current status |
| 🧠 **[CPU & Memory Subsystem](CPU-and-Memory-Subsystem)** | 64-bit ELF/SELF parsing, guest address space allocation, multi-threading & VEH |
| 🏛️ **[Syscalls & System Libraries](Syscalls-and-System-Libraries)** | FreeBSD ABI mapping, syscall trap dispatcher, `libkernel` & `libSce*` module stubs |
| 🎮 **[GPU & Shader Recompilation](GPU-and-Shader-Recompilation)** | GNM PM4 Type-3 packet parser, Vulkan 1.3 backend, RDNA2 to SPIR-V 1.5 recompiler |
| 🔊 **[Audio & Input Subsystems](Audio-and-Input-Subsystems)** | Tempest 3D Audio Tech, 48kHz PCM sample routing, DualSense HID driver & `libScePad` |
| 📁 **[Filesystem & Storage](Filesystem-and-Decompression)** | Virtual Filesystem (VFS) mounts, SaveData container manager, Kraken/Oodle decompression |
| 🎯 **[Compatibility & Triage](Compatibility-and-Stub-Triage)** | Per-title status database, automated missing symbol triage logger, regression test suite |
| ⚡ **[Performance & Optimization](Performance-and-Optimization)** | Persistent disk PSO cache, async shader compilation worker pool, frame pacing & FSR |
| 🛠️ **[Building & Debugging](Building-and-Debugging)** | System prerequisites, MSVC/CMake build instructions, Catch2 testing, ImGui UI guide |

---

## 🌟 Project Goals & Clean-Room Governance

1. **Clean-Room Engineering**: Developed strictly using public FreeBSD kernel documentation, AMD RDNA2 ISA specification manuals, and open ABI standards — **zero proprietary Sony code or copyrighted headers**.
2. **High-Performance Architecture**: Native 64-bit translation using C++20, Vulkan 1.3 state caching, multi-threaded submission, and low-latency audio/input queues.
3. **Transparency & Tooling**: Built-in visual Debug Shell UI providing real-time telemetry, memory inspection, symbol triage logging, and shader disassembly.
