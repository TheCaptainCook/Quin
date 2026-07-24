#include "input/input_manager.hpp"
#include "core/logging.hpp"

namespace quin::input {

InputManager::InputManager() = default;

InputManager::~InputManager() {
    shutdown();
}

bool InputManager::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    m_initialized = true;
    QUIN_LOG_INFO("InputManager Initialized — DualSense HID Controller & Keyboard Fallback Subsystem.");
    return true;
}

void InputManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    m_pads.clear();
    m_initialized = false;
    QUIN_LOG_INFO("InputManager Shutdown cleanly.");
}

PadHandle InputManager::open_pad(int32_t user_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PadHandle handle = user_id;

    PadState state{};
    state.handle = handle;
    state.is_connected = true;
    state.lightbar_color = ColorRgb{0, 102, 255}; // DualSense Cyan/Blue

    m_pads[handle] = state;
    QUIN_LOG_INFO("InputManager: Opened DualSense Controller Pad #{} for User ID {}", handle, user_id);
    return handle;
}

bool InputManager::close_pad(PadHandle handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pads.find(handle);
    if (it != m_pads.end()) {
        m_pads.erase(it);
        QUIN_LOG_INFO("InputManager: Closed Controller Pad #{}", handle);
        return true;
    }
    return false;
}

PadState InputManager::read_pad_state(PadHandle handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pads.find(handle);
    if (it != m_pads.end()) {
        return it->second;
    }
    return PadState{};
}

void InputManager::set_button_state(PadHandle handle, uint32_t button, bool pressed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pads.find(handle);
    if (it != m_pads.end()) {
        if (pressed) {
            it->second.buttons |= button;
        } else {
            it->second.buttons &= ~button;
        }
    }
}

void InputManager::set_analog_sticks(PadHandle handle, int8_t lx, int8_t ly, int8_t rx, int8_t ry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pads.find(handle);
    if (it != m_pads.end()) {
        it->second.left_stick_x = lx;
        it->second.left_stick_y = ly;
        it->second.right_stick_x = rx;
        it->second.right_stick_y = ry;
    }
}

void InputManager::set_lightbar(PadHandle handle, uint8_t r, uint8_t g, uint8_t b) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pads.find(handle);
    if (it != m_pads.end()) {
        it->second.lightbar_color = ColorRgb{r, g, b};
    }
}

void InputManager::set_vibration(PadHandle handle, uint8_t small_motor, uint8_t large_motor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pads.find(handle);
    if (it != m_pads.end()) {
        it->second.vibration = VibrationState{small_motor, large_motor};
    }
}

size_t InputManager::get_connected_pads_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pads.size();
}

std::vector<PadState> InputManager::get_all_pads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PadState> result;
    for (const auto& [handle, state] : m_pads) {
        result.push_back(state);
    }
    return result;
}

} // namespace quin::input
