#include "kernel/module_manager.hpp"
#include "audio/audio_engine.hpp"
#include "core/logging.hpp"

namespace quin::kernel {

static quin::audio::AudioEngine g_audio_engine;

void register_sce_audio_out(LibKernel& kernel) {
    g_audio_engine.initialize();

    kernel.register_stub("sceAudioOutInit", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceAudioOut: sceAudioOutInit() called.");
        return 0; // SCE_OK
    });

    kernel.register_stub("sceAudioOutOpen", [](uint64_t user_id, uint64_t type, uint64_t index, uint64_t param) -> int64_t {
        quin::audio::AudioOutPortConfig config{};
        config.channel_count = 2;
        config.sample_rate = 48000;
        quin::audio::AudioPortHandle handle = g_audio_engine.open_port(config);
        QUIN_LOG_INFO("libSceAudioOut: sceAudioOutOpen(user={}, type={}, index={}, param={}) -> Port {}",
                      user_id, type, index, param, handle);
        return handle;
    });

    kernel.register_stub("sceAudioOutOutput", [](uint64_t handle, uint64_t ptr, uint64_t, uint64_t) -> int64_t {
        if (ptr == 0) return 0;
        g_audio_engine.submit_pcm_samples(static_cast<quin::audio::AudioPortHandle>(handle), reinterpret_cast<const void*>(ptr), 256);
        return 0;
    });

    kernel.register_stub("sceAudioOutSetVolume", [](uint64_t handle, uint64_t left, uint64_t right, uint64_t) -> int64_t {
        float l_vol = static_cast<float>(left) / 32768.0f;
        float r_vol = static_cast<float>(right) / 32768.0f;
        g_audio_engine.set_volume(static_cast<quin::audio::AudioPortHandle>(handle), l_vol, r_vol);
        return 0;
    });

    kernel.register_stub("sceAudioOutClose", [](uint64_t handle, uint64_t, uint64_t, uint64_t) -> int64_t {
        g_audio_engine.close_port(static_cast<quin::audio::AudioPortHandle>(handle));
        return 0;
    });

    QUIN_LOG_INFO("libSceAudioOut system module stubs registered.");
}

} // namespace quin::kernel
