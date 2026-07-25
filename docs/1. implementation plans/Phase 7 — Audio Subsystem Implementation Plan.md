# Phase 7 — Audio Subsystem Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 7 — Audio Subsystem** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **`libSceAudioOut` Module**: Implements core stubs (`sceAudioOutInit`, `sceAudioOutOpen`, `sceAudioOutOutput`, `sceAudioOutSetVolume`, `sceAudioOutClose`) in system library management.
> - **Tempest 3D AudioTech & PCM Engine**: Audio ring buffer queue managing 48kHz multi-channel PCM sample streams with volume panning and 7.1/5.1 downmixing.
> - **Host SDL2 Audio Backend**: Routes guest audio buffers directly to host audio output devices via SDL2 audio APIs (`SDL_OpenAudioDevice`, `SDL_QueueAudio`).

## Proposed Changes

---

### Audio Subsystem (`src/audio/`)

#### [NEW] [audio_types.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/audio/audio_types.hpp)
- `AudioFormat`, `AudioChannelLayout`, `AudioOutPort`, and `PcmBuffer` structures.

#### [NEW] [audio_engine.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/audio/audio_engine.hpp) & [audio_engine.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/audio/audio_engine.cpp)
- `AudioEngine` managing audio port allocation, lock-free PCM ring buffers, 7.1 to stereo downmixing, volume control, and host SDL2 audio device streaming.

---

### System Library Stubs (`src/kernel/modules/`)

#### [NEW] [sce_audio_out.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/modules/sce_audio_out.cpp)
- `libSceAudioOut` module implementation binding PS5 audio output API calls to `AudioEngine`.

#### [MODIFY] [module_manager.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/module_manager.hpp) & [module_manager.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/module_manager.cpp)
- Register `libSceAudioOut` module stubs.

---

### Debug UI & Integration (`src/gui/`)

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp) & [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Audio Subsystem & Tempest 3D" ImGui panel rendering active audio ports, sample rates, channel configurations, volume sliders, and buffer telemetry.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include audio sources in `quin-core`.
- Add `tests/unit/test_audio_subsystem.cpp` target to `quin-tests`.

#### [NEW] [test_audio_subsystem.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_audio_subsystem.cpp)
- Catch2 unit tests for `libSceAudioOut` symbol dispatch, PCM sample buffering, volume panning, downmixing, and `AudioEngine` state.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 7 as ✅ **Complete** and Phase 8 as 🟡 **Next** (before git push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, view audio ports, volume metering sliders, and host audio status in the ImGui debug shell.
