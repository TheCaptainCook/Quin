#ifndef QUIN_INPUT_INPUT_MANAGER_HPP
#define QUIN_INPUT_INPUT_MANAGER_HPP

#include "input/input_types.hpp"
#include <unordered_map>
#include <mutex>
#include <vector>

namespace quin::input {

class InputManager {
public:
    InputManager();
    ~InputManager();

    bool initialize();
    void shutdown();

    PadHandle open_pad(int32_t user_id);
    bool close_pad(PadHandle handle);

    PadState read_pad_state(PadHandle handle);
    void set_button_state(PadHandle handle, uint32_t button, bool pressed);
    void set_analog_sticks(PadHandle handle, int8_t lx, int8_t ly, int8_t rx, int8_t ry);
    void set_lightbar(PadHandle handle, uint8_t r, uint8_t g, uint8_t b);
    void set_vibration(PadHandle handle, uint8_t small_motor, uint8_t large_motor);

    size_t get_connected_pads_count() const;
    std::vector<PadState> get_all_pads() const;

private:
    bool m_initialized{false};
    std::unordered_map<PadHandle, PadState> m_pads;
    mutable std::mutex m_mutex;
};

} // namespace quin::input

#endif // QUIN_INPUT_INPUT_MANAGER_HPP
