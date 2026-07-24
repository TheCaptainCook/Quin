#include "kernel/module_manager.hpp"
#include "input/input_manager.hpp"
#include "core/logging.hpp"

namespace quin::kernel {

static quin::input::InputManager g_input_manager;

void register_sce_pad(LibKernel& kernel) {
    g_input_manager.initialize();

    kernel.register_stub("scePadInit", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libScePad: scePadInit() called.");
        return 0; // SCE_OK
    });

    kernel.register_stub("scePadOpen", [](uint64_t user_id, uint64_t type, uint64_t index, uint64_t param) -> int64_t {
        quin::input::PadHandle handle = g_input_manager.open_pad(static_cast<int32_t>(user_id));
        QUIN_LOG_INFO("libScePad: scePadOpen(user={}, type={}, index={}, param={}) -> Pad {}",
                      user_id, type, index, param, handle);
        return handle;
    });

    kernel.register_stub("scePadReadState", [](uint64_t handle, uint64_t out_state_ptr, uint64_t, uint64_t) -> int64_t {
        if (out_state_ptr == 0) return 0;
        quin::input::PadState pad = g_input_manager.read_pad_state(static_cast<quin::input::PadHandle>(handle));
        (void)pad;
        return 0;
    });

    kernel.register_stub("scePadSetVibration", [](uint64_t handle, uint64_t small_m, uint64_t large_m, uint64_t) -> int64_t {
        g_input_manager.set_vibration(
            static_cast<quin::input::PadHandle>(handle),
            static_cast<uint8_t>(small_m),
            static_cast<uint8_t>(large_m)
        );
        return 0;
    });

    kernel.register_stub("scePadSetLightBar", [](uint64_t handle, uint64_t rgb_ptr, uint64_t, uint64_t) -> int64_t {
        g_input_manager.set_lightbar(static_cast<quin::input::PadHandle>(handle), 0, 102, 255);
        return 0;
    });

    kernel.register_stub("scePadClose", [](uint64_t handle, uint64_t, uint64_t, uint64_t) -> int64_t {
        g_input_manager.close_pad(static_cast<quin::input::PadHandle>(handle));
        return 0;
    });

    QUIN_LOG_INFO("libScePad system module stubs registered.");
}

} // namespace quin::kernel
