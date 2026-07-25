#include "input/input_manager.hpp"
#include "core/logging.hpp"
#include <SDL.h>

namespace quin::input {

InputManager::InputManager() = default;

InputManager::~InputManager() {
    shutdown();
}

bool InputManager::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    // Initialize SDL GameController subsystem if not already done
    if (!(SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER)) {
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
            QUIN_LOG_WARN("InputManager: SDL_InitSubSystem(GAMECONTROLLER) failed: {}. Virtual-only mode.", SDL_GetError());
        }
    }

    // Scan for connected controllers
    int num_joysticks = SDL_NumJoysticks();
    QUIN_LOG_INFO("InputManager: Detected {} joystick(s)", num_joysticks);

    for (int i = 0; i < num_joysticks && i < 4; ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* gc = SDL_GameControllerOpen(i);
            if (gc) {
                PadHandle handle = i;
                PadState state{};
                state.handle = handle;
                state.is_connected = true;
                state.lightbar_color = ColorRgb{0, 102, 255}; // DualSense Cyan/Blue

                m_pads[handle] = state;
                m_sdl_controllers[handle] = gc;
                m_has_real_controllers = true;

                const char* name = SDL_GameControllerName(gc);
                QUIN_LOG_INFO("InputManager: Opened Controller #{} — '{}'", handle, name ? name : "Unknown");

                // Set initial lightbar color if supported (DualSense)
                SDL_GameControllerSetLED(gc, 0, 102, 255);
            }
        }
    }

    m_initialized = true;
    QUIN_LOG_INFO("InputManager Initialized — {} real controller(s) | DualSense HID & Keyboard Fallback.",
                  m_sdl_controllers.size());
    return true;
}

void InputManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    for (auto& [handle, gc] : m_sdl_controllers) {
        if (gc) {
            SDL_GameControllerClose(gc);
        }
    }
    m_sdl_controllers.clear();
    m_pads.clear();
    m_has_real_controllers = false;
    m_initialized = false;
    QUIN_LOG_INFO("InputManager Shutdown cleanly.");
}

void InputManager::poll_input() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    // Check for hotplug: new controllers connected
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks && i < 4; ++i) {
        if (SDL_IsGameController(i) && m_sdl_controllers.find(i) == m_sdl_controllers.end()) {
            SDL_GameController* gc = SDL_GameControllerOpen(i);
            if (gc) {
                PadHandle handle = i;
                PadState state{};
                state.handle = handle;
                state.is_connected = true;
                state.lightbar_color = ColorRgb{0, 102, 255};

                m_pads[handle] = state;
                m_sdl_controllers[handle] = gc;
                m_has_real_controllers = true;

                const char* name = SDL_GameControllerName(gc);
                QUIN_LOG_INFO("InputManager: Hotplug — Controller #{} '{}' connected", handle, name ? name : "Unknown");
            }
        }
    }

    // Poll each connected controller
    for (auto& [handle, gc] : m_sdl_controllers) {
        if (!gc || !SDL_GameControllerGetAttached(gc)) {
            m_pads[handle].is_connected = false;
            continue;
        }

        auto& pad = m_pads[handle];
        pad.is_connected = true;

        // Read analog sticks (SDL range: -32768..32767 → map to int8_t -128..127)
        int16_t lx_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t ly_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
        int16_t rx_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX);
        int16_t ry_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY);

        pad.left_stick_x  = static_cast<int8_t>(lx_raw >> 8);
        pad.left_stick_y  = static_cast<int8_t>(ly_raw >> 8);
        pad.right_stick_x = static_cast<int8_t>(rx_raw >> 8);
        pad.right_stick_y = static_cast<int8_t>(ry_raw >> 8);

        // Read triggers (0..32767 → 0..255)
        int16_t l2_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        int16_t r2_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        pad.l2_analog = static_cast<uint8_t>((l2_raw >> 7) & 0xFF);
        pad.r2_analog = static_cast<uint8_t>((r2_raw >> 7) & 0xFF);

        // Read buttons → map to PS5 button bitmask
        uint32_t buttons = 0;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A))           buttons |= PAD_CROSS;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B))           buttons |= PAD_CIRCLE;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X))           buttons |= PAD_SQUARE;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y))           buttons |= PAD_TRIANGLE;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))  buttons |= PAD_L1;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) buttons |= PAD_R1;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSTICK))     buttons |= PAD_L3;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSTICK))    buttons |= PAD_R3;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_START))       buttons |= PAD_OPTIONS;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP))     buttons |= PAD_UP;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN))   buttons |= PAD_DOWN;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT))   buttons |= PAD_LEFT;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))  buttons |= PAD_RIGHT;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_TOUCHPAD))    buttons |= PAD_TOUCHPAD;

        // L2/R2 digital from analog threshold
        if (pad.l2_analog > 128) buttons |= PAD_L2;
        if (pad.r2_analog > 128) buttons |= PAD_R2;

        pad.buttons = buttons;
    }
}

PadHandle InputManager::open_pad(int32_t user_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PadHandle handle = user_id;

    // If a real controller is already mapped to this handle, keep it
    if (m_pads.find(handle) != m_pads.end()) {
        return handle;
    }

    // Try to open a real controller for this user
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER) {
        int joystick_idx = user_id;
        if (joystick_idx < SDL_NumJoysticks() && SDL_IsGameController(joystick_idx)) {
            SDL_GameController* gc = SDL_GameControllerOpen(joystick_idx);
            if (gc) {
                m_sdl_controllers[handle] = gc;
                m_has_real_controllers = true;
                QUIN_LOG_INFO("InputManager: Opened real controller for Pad #{}", handle);
            }
        }
    }

    PadState state{};
    state.handle = handle;
    state.is_connected = true;
    state.lightbar_color = ColorRgb{0, 102, 255};

    m_pads[handle] = state;
    QUIN_LOG_INFO("InputManager: Opened DualSense Controller Pad #{} for User ID {}", handle, user_id);
    return handle;
}

bool InputManager::close_pad(PadHandle handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto gc_it = m_sdl_controllers.find(handle);
    if (gc_it != m_sdl_controllers.end()) {
        if (gc_it->second) {
            SDL_GameControllerClose(gc_it->second);
        }
        m_sdl_controllers.erase(gc_it);
    }

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

    // Apply to real controller if available
    auto gc_it = m_sdl_controllers.find(handle);
    if (gc_it != m_sdl_controllers.end() && gc_it->second) {
        SDL_GameControllerSetLED(gc_it->second, r, g, b);
    }
}

void InputManager::set_vibration(PadHandle handle, uint8_t small_motor, uint8_t large_motor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pads.find(handle);
    if (it != m_pads.end()) {
        it->second.vibration = VibrationState{small_motor, large_motor};
    }

    // Apply rumble to real controller
    auto gc_it = m_sdl_controllers.find(handle);
    if (gc_it != m_sdl_controllers.end() && gc_it->second) {
        // SDL rumble takes uint16 (0-65535), our motors are uint8 (0-255)
        uint16_t low_freq  = static_cast<uint16_t>(large_motor) * 257; // Scale 0-255 → 0-65535
        uint16_t high_freq = static_cast<uint16_t>(small_motor) * 257;
        SDL_GameControllerRumble(gc_it->second, low_freq, high_freq, 100); // 100ms duration
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
