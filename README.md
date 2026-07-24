# Quin

> A lean x86-64 translation layer and PS5 console emulator built for high performance, modern graphics translation, and modular architecture.

---

## 🚀 Features & Architecture

- **Clean-Room Engineering**: Developed strictly using public specifications, FreeBSD syscall standards, and AMD RDNA2 ISA manuals.
- **Cross-Platform Foundation**: Native C++20 CMake build supporting Windows, Linux, and macOS.
- **Multi-Threaded CPU & TLS Model**: Guest thread context management with thread-local storage (TLS) isolation and stack guard page protection.
- **Native Exception Interception**: Windows Vectored Exception Handler (VEH) and POSIX signal translation capturing guest access violations without crashing the host process.
- **Dynamic Guest Virtual Memory**: Page-aligned `mmap`, `munmap`, and `mprotect` memory allocation matching PS5 user-space layout conventions.
- **Interactive ImGui Debug Shell**: Built-in ImGui workspace featuring real-time `spdlog` console streaming, ELF loader state, Threads & TLS inspector, and telemetry.
- **Modern Dependency Management**: Self-contained CMake FetchContent setup for `spdlog`, `Catch2`, `SDL2`, and `Dear ImGui`.

---

## 📜 Development Roadmap Progress

| Phase | Goal | Status |
| :--- | :--- | :---: |
| **Phase 0 — Foundations & Tooling** | Build system, CI matrix, clean-room policy, ImGui debug shell, logging | ✅ **Complete** |
| **Phase 1 — Executable Loading** | SELF/ELF parser, guest address space allocator, libkernel stubs | ✅ **Complete** |
| **Phase 2 — CPU Execution & Memory Model** | Multi-threaded execution harness, exception translation, TLS & guard pages | ✅ **Complete** |
| **Phase 3 — Syscalls & System Libraries** | FreeBSD syscall dispatch and core `libSce*` system libraries | 🟡 **Next** |
| **Phase 4 — Filesystem & Decompression** | VFS layer and Kraken/Oodle asset decompression pipeline | ⏳ Pending |
| **Phase 5 — GPU Command Processing** | GNM command buffer parsing & Vulkan 1.3 pipeline translation | ⏳ Pending |
| **Phase 6 — Shader Recompilation** | RDNA2 ISA to SPIR-V shader translator | ⏳ Pending |
| **Phase 7 — Audio Subsystem** | Tempest 3D Audio & PCM audio backend routing | ⏳ Pending |
| **Phase 8 — Input Subsystem** | DualSense HID controller mapping | ⏳ Pending |
| **Phase 9 & 10 — Compatibility & Performance** | Title library expansion and high-framerate optimizations | ⏳ Ongoing |

Detailed roadmap available in [`docs/4. instructions/ps5-emultor.md`](docs/4.%20instructions/ps5-emultor.md).

---

## 🛠️ Building & Running

### Prerequisites
- C++20 compliant compiler (MSVC 2022 / GCC 12+ / Clang 15+)
- CMake 3.22 or higher
- Git

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/TheCaptainCook/Quin.git
cd Quin

# Configure build with CMake (automatically fetches third-party dependencies)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build executable and unit tests
cmake --build build --config Release

# Run unit tests
ctest --test-dir build -C Release --output-on-failure

# Launch Quin Debug Shell
./build/bin/Release/quin.exe
```

---

## 📄 Licensing & Governance

- Licensed under the **[BSD 3-Clause License](LICENSE)**.
- Review our **[Clean-Room Policy](docs/3.%20others/clean-room-policy.md)** and **[Technical References](docs/3.%20others/references.md)**.
