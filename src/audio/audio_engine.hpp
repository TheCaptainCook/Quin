#ifndef QUIN_AUDIO_AUDIO_ENGINE_HPP
#define QUIN_AUDIO_AUDIO_ENGINE_HPP

#include "audio/audio_types.hpp"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

struct SDL_AudioSpec;

namespace quin::audio {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool initialize();
    void shutdown();

    AudioPortHandle open_port(const AudioOutPortConfig& config);
    bool close_port(AudioPortHandle handle);
    bool set_volume(AudioPortHandle handle, float left, float right);

    int64_t submit_pcm_samples(AudioPortHandle handle, const void* pcm_data, size_t num_samples);

    size_t get_open_ports_count() const;
    std::vector<AudioPortState> get_active_ports() const;
    uint64_t get_total_samples_processed() const { return m_total_samples_processed; }
    bool is_initialized() const { return m_initialized; }
    bool has_real_audio() const { return m_sdl_audio_device_id != 0; }

private:
    bool m_initialized{false};
    AudioPortHandle m_next_handle{100};
    std::unordered_map<AudioPortHandle, AudioPortState> m_ports;
    uint64_t m_total_samples_processed{0};
    mutable std::mutex m_mutex;

    // SDL Audio device
    uint32_t m_sdl_audio_device_id{0};
    uint32_t m_device_sample_rate{48000};
    uint32_t m_device_channels{2};
};

} // namespace quin::audio

#endif // QUIN_AUDIO_AUDIO_ENGINE_HPP
