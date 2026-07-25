#ifndef QUIN_INPUT_INPUT_MANAGER_HPP
#define QUIN_INPUT_INPUT_MANAGER_HPP

#include "input/input_types.hpp"
#include <unordered_map>
#include <mutex>
#include <vector>

struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

namespace quin::input {

class InputManager {
public:
    InputManager();
    ~InputManager();

    bool initialize();
    void shutdown();

    // Poll all connected controllers for live input
    void poll_input();

    PadHandle open_pad(int32_t user_id);
    bool close_pad(PadHandle handle);

    PadState read_pad_state(PadHandle handle);
    void set_button_state(PadHandle handle, uint32_t button, bool pressed);
    void set_analog_sticks(PadHandle handle, int8_t lx, int8_t ly, int8_t rx, int8_t ry);
    void set_lightbar(PadHandle handle, uint8_t r, uint8_t g, uint8_t b);
    void set_vibration(PadHandle handle, uint8_t small_motor, uint8_t large_motor);

    size_t get_connected_pads_count() const;
    std::vector<PadState> get_all_pads() const;
    bool has_real_controllers() const { return m_has_real_controllers; }

private:
    bool m_initialized{false};
    bool m_has_real_controllers{false};
    std::unordered_map<PadHandle, PadState> m_pads;
    std::unordered_map<PadHandle, SDL_GameController*> m_sdl_controllers;
    mutable std::mutex m_mutex;
};

} // namespace quin::input

#endif // QUIN_INPUT_INPUT_MANAGER_HPP
