# Phase 8 — Input Subsystem Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 8 — Input Subsystem** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **`libScePad` System Module**: Implements core PS5 controller stubs (`scePadInit`, `scePadOpen`, `scePadReadState`, `scePadSetVibration`, `scePadSetLightBar`, `scePadClose`).
> - **DualSense HID & Host Input Layer**: Maps native DualSense HID reports and host SDL2 GameController / Keyboard events into normalized `PadState` structures (buttons, analog sticks, L2/R2 triggers).
> - **Lightbar & Rumble Feedback**: Supports DualSense RGB LED lightbar colors and haptic rumble motor feedback calls.

## Proposed Changes

---

### Input Subsystem (`src/input/`)

#### [NEW] [input_types.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/input/input_types.hpp)
- `PadButton` bitmask flags, `PadState` struct (analog sticks, triggers, buttons), `TouchData`, `MotionData`, and RGB lightbar color representations.

#### [NEW] [input_manager.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/input/input_manager.hpp) & [input_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/input/input_manager.cpp)
- `InputManager` managing SDL2 controller polling, keyboard fallback mapping, button state updates, and rumble/lightbar state.

---

### System Library Stubs (`src/kernel/modules/`)

#### [NEW] [sce_pad.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_pad.cpp)
- `libScePad` module implementation exposing PS5 pad control entry points to guest processes.

#### [MODIFY] [module_manager.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/module_manager.hpp) & [module_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/module_manager.cpp)
- Register `libScePad` module stubs.

---

### Debug UI & Integration (`src/gui/`)

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp) & [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Input Subsystem & DualSense" ImGui panel rendering active controllers, button press indicators, analog stick X/Y positions, trigger values, lightbar color preview, and virtual button test triggers.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include input sources in `quin-core`.
- Add `tests/unit/test_input_subsystem.cpp` target to `quin-tests`.

#### [NEW] [test_input_subsystem.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_input_subsystem.cpp)
- Catch2 unit tests for `libScePad` symbol dispatch, button state bitmasking, analog normalization, keyboard fallback, and `InputManager`.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 8 as ✅ **Complete** and Phase 9 as 🟡 **Next** (before git push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, view controller status, press keys / buttons, and observe real-time input indicators in the ImGui debug shell.
