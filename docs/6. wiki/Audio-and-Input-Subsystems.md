# 🔊 Audio & Input Subsystems — Quin PS5 Emulator

## 1. Tempest 3D Audio Subsystem (`src/audio/`)

### Audio Engine Architecture (`audio_engine.hpp`/`cpp`)
- `AudioEngine` routes 48 kHz multi-channel PCM sample streams through real host SDL2 audio devices via `SDL_OpenAudioDevice` and `SDL_QueueAudio`.
- Supports stereo S16LE output with F32→S16 format conversion and per-channel volume scaling.
- Operates in silent fallback mode if no host audio hardware is present.

```
+-----------------------------------------------------------------+
|                         AudioEngine                             |
|                                                                 |
|  +-------------------+  +-------------------+  +-------------+  |
|  | Guest Audio Port  |  | PCM Sample Buffer |  | Volume &    |  |
|  |  Config (48 kHz)  |  |   Ring Queues     |  | Panning     |  |
|  +-------------------+  +-------------------+  +-------------+  |
|                                    |                            |
|                                    v                            |
|                        +-----------------------+                |
|                        | SDL2 Master Audio Out |                |
|                        | (SDL_QueueAudio S16)  |                |
|                        +-----------------------+                |
+-----------------------------------------------------------------+
```

### `libSceAudioOut` System Library Module (`src/kernel/modules/sce_audio_out.cpp`)
- `sceAudioOutInit`: Initializes audio subsystem ring buffers.
- `sceAudioOutOpen`: Opens an audio port with target channel count and sample rate.
- `sceAudioOutOutput`: Submits PCM sample buffers to host SDL2 audio stream.
- `sceAudioOutSetVolume`: Adjusts port volume gain vectors.
- `sceAudioOutClose`: Releases open audio port handles.

---

## 2. Input Subsystem & DualSense Driver (`src/input/`)

### DualSense HID Controller Driver (`input_manager.hpp`/`cpp`)
- `InputManager` communicates directly with PS5 DualSense controllers and host gamepads via SDL2 GameController API (`SDL_GameControllerOpen`, `SDL_GameControllerGetAxis`, `SDL_GameControllerGetButton`).
- Features controller hotplug detection.
- Normalizes controller inputs into `PadState` structures:
  - **Digital Buttons**: `PAD_CROSS`, `PAD_CIRCLE`, `PAD_SQUARE`, `PAD_TRIANGLE`, `PAD_L1`, `PAD_R1`, `PAD_L2`, `PAD_R2`, `PAD_L3`, `PAD_R3`, `PAD_OPTIONS`, `PAD_TOUCHPAD`, `PAD_UP`, `PAD_DOWN`, `PAD_LEFT`, `PAD_RIGHT`.
  - **Analog Sticks**: Left and Right stick X/Y axes normalized to range `[-128, +127]`.
  - **Lightbar RGB**: `ColorRgb` values (`0-255`) set via `SDL_GameControllerSetLED`.
  - **Vibration Feedback**: Dual haptic motor state (`small_motor`, `large_motor`) driven via `SDL_GameControllerRumble`.

### `libScePad` System Library Module (`src/kernel/modules/sce_pad.cpp`)
- `scePadInit`: Initializes controller subsystem.
- `scePadOpen`: Registers connected DualSense controller handles (`PadHandle`).
- `scePadReadState`: Returns current button masks and analog stick states to guest code.
- `scePadSetVibration`: Sets haptic rumble motor intensities.
- `scePadSetLightBar`: Modifies controller RGB lightbar colors.
- `scePadClose`: Closes pad handle connections.
