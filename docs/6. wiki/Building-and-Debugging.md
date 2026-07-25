# 🛠️ Building & Debugging — Quin PS5 Emulator

## 1. System Requirements & Dependencies

### Prerequisites
- **Operating System**: Windows 10/11 (64-bit), Linux (Ubuntu 22.04+), or macOS (12+)
- **Compiler**: C++20 compliant compiler (MSVC 2022, GCC 12+, Clang 15+)
- **Build Tool**: CMake 3.22+
- **Graphics Card (Optional)**: Vulkan 1.3 compatible GPU for hardware GPU detection.

### Automatic Dependencies (FetchContent + find_package)
- `spdlog` (v1.14.1): Logging framework.
- `Catch2` (v3.5.2): Unit testing framework.
- `SDL2` (release-2.30.1): Windowing, OpenGL context, audio output, controller input.
- `Dear ImGui` (v1.90.5): Graphical Debug Shell UI.
- `Vulkan SDK` (Optional): Detected via `find_package(Vulkan QUIET)`.

---

## 2. Build Instructions

### Command Line Build (CMake)
```bash
# 1. Clone repository
git clone https://github.com/TheCaptainCook/Quin.git
cd Quin

# 2. Configure project
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

# 3. Compile static library, main executable, and test suite
cmake --build build --config Release

# 4. Run Catch2 test suite
ctest --test-dir build -C Release --output-on-failure

# 5. Launch application
./build/bin/Release/quin.exe
```

---

## 3. ImGui Debug Shell UI Guide

The Debug Shell UI automatically scales to fit 100% of host window resolution with 6 workspace tabs:
1. **📊 Main Dashboard**: Frame rate stats, memory usage, ring-buffer log console.
2. **⚡ CPU Execution & Kernel**: Active thread inspector, TLS addresses, register states, syscall call counts.
3. **🎮 GPU & Shader Pipeline**: Vulkan device info, GNM PM4 packet counts, cached PSOs, shader cache inspector.
4. **🔊 Audio & Input**: Active audio ports, PCM sample counters, connected controller status, DualSense lightbar/rumble test triggers.
5. **📁 Storage & VFS**: Active VFS mount table, open file handles, SaveData container inspector.
6. **🎯 Compatibility Matrix**: Per-title status ratings, missing symbol call counts, automated regression test runner.
