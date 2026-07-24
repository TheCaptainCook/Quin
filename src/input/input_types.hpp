#ifndef QUIN_INPUT_INPUT_TYPES_HPP
#define QUIN_INPUT_INPUT_TYPES_HPP

#include <cstdint>
#include <string>

namespace quin::input {

using PadHandle = int32_t;
constexpr PadHandle INVALID_PAD_HANDLE = -1;

enum PadButton : uint32_t {
    PAD_L3       = 0x00000002,
    PAD_R3       = 0x00000004,
    PAD_OPTIONS  = 0x00000008,
    PAD_UP       = 0x00000010,
    PAD_RIGHT    = 0x00000020,
    PAD_DOWN     = 0x00000040,
    PAD_LEFT     = 0x00000080,
    PAD_L2       = 0x00000100,
    PAD_R2       = 0x00000200,
    PAD_L1       = 0x00000400,
    PAD_R1       = 0x00000800,
    PAD_TRIANGLE = 0x00001000,
    PAD_CIRCLE   = 0x00002000,
    PAD_CROSS    = 0x00004000,
    PAD_SQUARE   = 0x00008000,
    PAD_TOUCHPAD = 0x00100000
};

struct ColorRgb {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{255}; // Default DualSense Blue
};

struct VibrationState {
    uint8_t small_motor{0};
    uint8_t large_motor{0};
};

struct PadState {
    PadHandle handle{INVALID_PAD_HANDLE};
    uint32_t buttons{0};
    int8_t left_stick_x{0};
    int8_t left_stick_y{0};
    int8_t right_stick_x{0};
    int8_t right_stick_y{0};
    uint8_t l2_analog{0};
    uint8_t r2_analog{0};
    bool is_connected{true};
    ColorRgb lightbar_color;
    VibrationState vibration;
};

} // namespace quin::input

#endif // QUIN_INPUT_INPUT_TYPES_HPP
