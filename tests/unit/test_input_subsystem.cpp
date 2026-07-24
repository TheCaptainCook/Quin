#include <catch2/catch_test_macros.hpp>
#include "input/input_types.hpp"
#include "input/input_manager.hpp"
#include "kernel/libkernel.hpp"
#include "kernel/module_manager.hpp"
#include "memory/address_space.hpp"

TEST_CASE("Input Manager Pad State & Controls", "[input][manager]") {
    quin::input::InputManager manager;
    REQUIRE(manager.initialize() == true);

    // 1. Open Pad
    quin::input::PadHandle pad = manager.open_pad(0);
    REQUIRE(pad == 0);
    REQUIRE(manager.get_connected_pads_count() == 1);

    // 2. Set & Read Buttons
    manager.set_button_state(pad, quin::input::PAD_CROSS, true);
    manager.set_button_state(pad, quin::input::PAD_TRIANGLE, true);

    quin::input::PadState state1 = manager.read_pad_state(pad);
    REQUIRE((state1.buttons & quin::input::PAD_CROSS) != 0);
    REQUIRE((state1.buttons & quin::input::PAD_TRIANGLE) != 0);
    REQUIRE((state1.buttons & quin::input::PAD_CIRCLE) == 0);

    // 3. Set Analog Sticks
    manager.set_analog_sticks(pad, 64, -128, 0, 127);
    quin::input::PadState state2 = manager.read_pad_state(pad);
    REQUIRE(state2.left_stick_x == 64);
    REQUIRE(state2.left_stick_y == -128);

    // 4. Set Lightbar & Vibration
    manager.set_lightbar(pad, 255, 0, 128);
    manager.set_vibration(pad, 100, 200);

    quin::input::PadState state3 = manager.read_pad_state(pad);
    REQUIRE(state3.lightbar_color.r == 255);
    REQUIRE(state3.vibration.small_motor == 100);

    // 5. Close Pad
    REQUIRE(manager.close_pad(pad) == true);
    REQUIRE(manager.get_connected_pads_count() == 0);
}

TEST_CASE("libScePad Symbol Registration & Dispatch", "[kernel][input]") {
    quin::memory::GuestAddressSpace memory;
    quin::kernel::LibKernel kernel;
    quin::kernel::SyscallDispatcher syscalls(memory);
    quin::kernel::ModuleManager module_mgr(kernel, syscalls);

    module_mgr.register_all_modules();

    REQUIRE(kernel.has_symbol("scePadInit") == true);
    REQUIRE(kernel.has_symbol("scePadOpen") == true);
    REQUIRE(kernel.has_symbol("scePadReadState") == true);

    int64_t init_ret = kernel.dispatch_symbol("scePadInit", 0);
    REQUIRE(init_ret == 0);

    int64_t pad_handle = kernel.dispatch_symbol("scePadOpen", 0, 0, 0, 0);
    REQUIRE(pad_handle == 0);

    int64_t close_ret = kernel.dispatch_symbol("scePadClose", pad_handle);
    REQUIRE(close_ret == 0);
}
