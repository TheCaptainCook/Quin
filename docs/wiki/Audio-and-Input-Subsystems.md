# 🔊 Audio & Input Subsystems — Quin PS5 Emulator

## 1. Tempest 3D Audio Subsystem (`src/audio/`)

### Audio Engine Architecture (`audio_engine.hpp`/`cpp`)
- `AudioEngine` simulates PS5 Tempest 3D AudioTech processing by routing 48 kHz multi-channel PCM sample streams through SDL2 audio device queues.
- Supports 2-channel (stereo), 5.1 surround, and 7.1 surround sound configurations.

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
- `InputManager` communicates directly with PS5 DualSense controllers over USB/Bluetooth HID reports and host SDL2 GameController interfaces.
- Normalizes controller inputs into `PadState` structures:
  - **Digital Buttons**: `PAD_CROSS`, `PAD_CIRCLE`, `PAD_SQUARE`, `PAD_TRIANGLE`, `PAD_L1`, `PAD_R1`, `PAD_L2_TRIGGER`, `PAD_R2_TRIGGER`, `PAD_L3`, `PAD_R3`, `PAD_OPTIONS`, `PAD_TOUCH_PAD`.
  - **Analog Sticks**: Left and Right stick X/Y axes normalized to range `[-128, +127]`.
  - **Lightbar RGB**: `ColorRgb` color values (`0-255`).
  - **Vibration Feedback**: Dual haptic motor state (`small_motor`, `large_motor`).

### `libScePad` System Library Module (`src/kernel/modules/sce_pad.cpp`)
- `scePadInit`: Initializes controller subsystem.
- `scePadOpen`: Registers connected DualSense controller handles (`PadHandle`).
- `scePadReadState`: Returns current button masks and analog stick states to guest code.
- `scePadSetVibration`: Sets haptic rumble motor intensities.
- `scePadSetLightBar`: Modifies controller RGB lightbar colors.
- `scePadClose`: Closes pad handle connections.
