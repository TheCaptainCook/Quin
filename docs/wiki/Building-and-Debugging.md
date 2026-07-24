# 🛠️ Building & Debugging — Quin PS5 Emulator

## 1. System Requirements

### Host Operating Systems
- **Windows**: Windows 10 / 11 (64-bit) with Visual Studio 2022 Build Tools
- **Linux**: Ubuntu 22.04 LTS or newer (GCC 12+, Clang 15+)
- **macOS**: macOS 12 Monterey or newer

### Graphics Hardware & Drivers
- **Vulkan Version**: Vulkan 1.3 Driver Support
- **GPUs Supported**: NVIDIA GTX 10-series or newer, AMD Radeon RX 500-series or newer, Intel Arc

---

## 2. Build Instructions

```bash
# 1. Clone the repository
git clone https://github.com/TheCaptainCook/Quin.git
cd Quin

# 2. Configure build with CMake (FetchContent auto-fetches dependencies)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build quin-core library, quin main executable, and quin-tests target
cmake --build build --config Release

# 4. Run full Catch2 unit test suite via CTest
ctest --test-dir build -C Release --output-on-failure

# 5. Launch Quin Debug Shell
./build/bin/Release/quin.exe
```

---

## 3. Catch2 Automated Unit Test Suite

The test suite executable `quin-tests.exe` includes 26 Catch2 test cases covering all 10 roadmap phases:
- `test_logging.cpp`: Logging initialization and ImGui sink log entry retrieval.
- `test_elf_loader.cpp`: 64-bit ELF parsing and segment loading.
- `test_cpu_memory.cpp`: Guest address space allocation, `mmap`, `mprotect`, guard pages, TLS isolation.
- `test_syscalls_modules.cpp`: FreeBSD syscall dispatcher and `libSce*` module symbol dispatch.
- `test_vfs_decompression.cpp`: VFS mount paths, SaveData container quota, Kraken byte-stream decoder.
- `test_gpu_vulkan.cpp`: GNM PM4 packet parser, Vulkan resource translation, PSO caching.
- `test_shader_recompiler.cpp`: RDNA2 ISA bytecode decoder, SPIR-V 1.5 emitter, in-memory shader cache.
- `test_audio_subsystem.cpp`: 48 kHz PCM sample routing, `libSceAudioOut` port stubs.
- `test_input_subsystem.cpp`: DualSense HID pad state reading, lightbar RGB, vibration, `libScePad` stubs.
- `test_compatibility_db.cpp`: Title database lookup, Markdown export, stub triage logger.
- `test_performance_60fps.cpp`: Persistent PSO disk serialization, async shader compiler pool, frame-pacing regulator.

---

## 4. Using the ImGui Debug Shell

The ImGui Debug Shell workspace dynamically scales to fit 100% of your screen resolution across 6 tabbed categories:

1. **📊 Main Dashboard**:
   - Live `spdlog` console streaming with log level checkboxes and search filters.
   - Executable loader status, entry point `0x0000000000400080ULL`, register inspector (`RIP`, `RSP`), and instruction step button.
   - System performance metrics (Framerate, Frame Time, Memory Mapped).

2. **⚡ CPU & Kernel**:
   - Active guest threads table (`TID`, `Name`, `State`, `RIP`, `RSP`, `TLS Base`).
   - Syscall dispatcher statistics table (`Syscall #`, `Name`, `Call Count`).

3. **🎮 GPU & Shaders**:
   - Vulkan physical device info, driver version, VRAM allocation.
   - Recompiled shaders table (`Hash Key`, `Stage`, `SPIR-V Words`, `RDNA2 Bytes`).

4. **🔊 Audio & Input**:
   - Open audio ports table (`Port #`, `Sample Rate`, `Channels`, `Submitted Samples`).
   - Connected DualSense controllers inspector (`Buttons Mask`, `Analog Sticks`, `Lightbar RGB`, test triggers).

5. **📁 Storage & VFS**:
   - Active VFS mount points (`/app0/`, `/data/`, `/system/`, `/savedata/`).
   - Open file handles table (`FD`, `Guest Path`, `Size`, `Position`).

6. **🎯 Compatibility & Triage**:
   - Playable / Perfect title status breakdown.
   - Top missing system symbols table ranked by call frequency.
   - Automated regression test suite launcher button.
