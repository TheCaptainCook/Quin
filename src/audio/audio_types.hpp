#ifndef QUIN_AUDIO_AUDIO_TYPES_HPP
#define QUIN_AUDIO_AUDIO_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace quin::audio {

using AudioPortHandle = int32_t;
constexpr AudioPortHandle INVALID_AUDIO_PORT = -1;

enum class AudioFormat : uint32_t {
    PCM_S16_LE = 0,
    PCM_F32_LE,
    PCM_S24_LE
};

enum class AudioChannelLayout : uint32_t {
    Stereo_2_0 = 2,
    Surround_5_1 = 6,
    Surround_7_1 = 8
};

struct AudioOutPortConfig {
    uint32_t sample_rate{48000};
    uint32_t channel_count{2};
    AudioFormat format{AudioFormat::PCM_S16_LE};
    uint32_t num_samples{256};
    float left_volume{1.0f};
    float right_volume{1.0f};
};

struct AudioPortState {
    AudioPortHandle handle{INVALID_AUDIO_PORT};
    AudioOutPortConfig config;
    bool is_open{false};
    uint64_t total_samples_submitted{0};
    uint64_t total_buffers_output{0};
};

} // namespace quin::audio

#endif // QUIN_AUDIO_AUDIO_TYPES_HPP
