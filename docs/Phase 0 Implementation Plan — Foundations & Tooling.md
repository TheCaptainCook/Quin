# Phase 0 Implementation Plan — Foundations & Tooling

Establish the foundation for **Quin** (the PS5 emulator) according to **Phase 0** of `docs/ps5-emultor.md`. This phase sets up project governance, build infrastructure, multi-platform CI, third-party dependencies, primary documentation references, and an interactive debug shell with real-time log streaming.

---

## User Review Required

> [!IMPORTANT]
> **Dependency Integration Strategy**: We recommend using `CMake FetchContent` (pinning modern stable tags for `spdlog`, `Catch2`, `SDL3`/`SDL2`, and `imgui`) so that the project builds out-of-the-box on Windows, Linux, and macOS without requiring developers to pre-install `vcpkg` or system libraries.
> 
> **License**: The repository currently holds a **BSD 3-Clause License**. Clean-room policy documentation will explicitly confirm BSD 3-Clause licensing and strict non-infringing guidelines.

---

## Open Questions

> [!NOTE]
> **GUI Backend Choice**: For maximum cross-platform hardware compatibility in early headless/VM CI environments as well as local dev, the debug shell can use SDL + OpenGL / Vulkan backends for ImGui. We will default to SDL + OpenGL/Vulkan windowing.

---

## Proposed Changes

### Governance & Documentation

#### [NEW] [clean-room-policy.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/docs/clean-room-policy.md)
- Establish strict clean-room policies: zero Sony code, no private SDKs, no key materials, no direct code copying from GPL emulator projects.
- Document copyright and reverse-engineering ethical boundaries.

#### [NEW] [references.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/docs/references.md)
- Primary reference catalog:
  - AMD RDNA2 ISA Specification
  - Khronos Vulkan 1.3 Specification
  - FreeBSD 12/13 System Call tables & ABI conventions
  - SELF, eboot.bin, and PKG file format technical writeups

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update README to detail build instructions (CMake), roadmap phase progress, architecture overview, and clean-room policy link.

---

### Build System & CI

#### [NEW] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Root CMake configuration requiring C++20.
- Configures target executables (`quin` GUI app, `quin-core` static lib, unit test suite).

#### [NEW] [cmake/Dependencies.cmake](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/cmake/Dependencies.cmake)
- `FetchContent` configuration for:
  - `spdlog` (Fast C++ logging framework)
  - `Catch2` (Unit testing framework)
  - `SDL3` / `SDL2` (Windowing and event handling)
  - `Dear ImGui` (Immediate mode GUI framework for emulator shell)

#### [NEW] [.github/workflows/ci.yml](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/.github/workflows/ci.yml)
- Multi-platform CI Matrix building and running unit tests on:
  - `windows-latest` (MSVC)
  - `ubuntu-latest` (GCC / Clang)
  - `macos-latest` (Apple Clang)

---

### Engine Core & Debug Shell

#### [NEW] [src/core/logging.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/core/logging.hpp)
#### [NEW] [src/core/logging.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/core/logging.cpp)
- Core logging module wrapping `spdlog`.
- Provides macros `QUIN_LOG_INFO`, `QUIN_LOG_WARN`, `QUIN_LOG_ERROR`, `QUIN_LOG_DEBUG`.

#### [NEW] [src/gui/imgui_sink.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/imgui_sink.hpp)
- Thread-safe custom `spdlog` sink capturing log entries into a circular buffer for rendering in the ImGui log console pane.

#### [NEW] [src/gui/debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp)
#### [NEW] [src/gui/debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- ImGui-based debug shell interface:
  - Menu Bar ("File" -> "Open ELF...", "Emulation" -> "Run/Pause/Reset", "Help" -> "About")
  - **Log Pane**: Real-time log message feed with level filtering (Info, Warning, Error, Debug), clear button, auto-scroll, and text search.
  - **ELF Status Pane**: Shows ELF file details (Entry point, header, architecture, load status).
  - **System Monitor Pane**: Framerate, frame time, thread counts, memory layout summary.

#### [NEW] [src/main.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/main.cpp)
- Application main loop:
  - Initializes SDL and graphics context.
  - Initializes ImGui and custom log sink.
  - Runs main render and event processing loop.
  - Clean shutdown sequence.

---

### Unit Tests

#### [NEW] [tests/unit/test_logging.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_logging.cpp)
- Catch2 unit test verifying log initialization, level filtering, and thread-safe custom sink behavior.

---

## Verification Plan

### Automated Tests
- Configure and build with CMake using MSVC BuildTools (`cmake -B build` and `cmake --build build`).
- Run Catch2 unit test suite (`ctest --test-dir build --output-on-failure`).

### Manual Verification
- Launch the compiled `quin` debug shell application executable.
- Verify:
  1. Main window opens cleanly with custom dark theme.
  2. ImGui Menu Bar and Panes (Log Console, ELF Status, System Monitor) render responsively.
  3. "Load ELF..." button opens file dialog/loader interface and logs the event in the Log Console pane.
  4. Closing window exits cleanly with no resource leaks.
