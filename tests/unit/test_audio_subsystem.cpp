#include <catch2/catch_test_macros.hpp>
#include "audio/audio_types.hpp"
#include "audio/audio_engine.hpp"
#include "kernel/libkernel.hpp"
#include "kernel/module_manager.hpp"
#include "memory/address_space.hpp"
#include <vector>

TEST_CASE("Audio Engine Port & PCM Routing", "[audio][engine]") {
    quin::audio::AudioEngine engine;
    REQUIRE(engine.initialize() == true);
    REQUIRE(engine.is_initialized() == true);

    quin::audio::AudioOutPortConfig config{};
    config.sample_rate = 48000;
    config.channel_count = 2;

    // 1. Open Port
    quin::audio::AudioPortHandle port = engine.open_port(config);
    REQUIRE(port >= 100);
    REQUIRE(engine.get_open_ports_count() == 1);

    // 2. Submit PCM Samples
    std::vector<int16_t> pcm_samples(512, 1000);
    int64_t submitted = engine.submit_pcm_samples(port, pcm_samples.data(), 256);
    REQUIRE(submitted == 256);
    REQUIRE(engine.get_total_samples_processed() == 256);

    // 3. Set Volume
    REQUIRE(engine.set_volume(port, 0.8f, 0.8f) == true);

    // 4. Close Port
    REQUIRE(engine.close_port(port) == true);
    REQUIRE(engine.get_open_ports_count() == 0);
}

TEST_CASE("libSceAudioOut Symbol Registration & Dispatch", "[kernel][audio]") {
    quin::memory::GuestAddressSpace memory;
    quin::kernel::LibKernel kernel;
    quin::kernel::SyscallDispatcher syscalls(memory);
    quin::kernel::ModuleManager module_mgr(kernel, syscalls);

    module_mgr.register_all_modules();

    REQUIRE(kernel.has_symbol("sceAudioOutInit") == true);
    REQUIRE(kernel.has_symbol("sceAudioOutOpen") == true);
    REQUIRE(kernel.has_symbol("sceAudioOutOutput") == true);

    int64_t init_ret = kernel.dispatch_symbol("sceAudioOutInit", 0);
    REQUIRE(init_ret == 0);

    int64_t port_handle = kernel.dispatch_symbol("sceAudioOutOpen", 1000, 0, 0, 0);
    REQUIRE(port_handle >= 100);

    int64_t close_ret = kernel.dispatch_symbol("sceAudioOutClose", port_handle);
    REQUIRE(close_ret == 0);
}
