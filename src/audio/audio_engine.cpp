#include "audio/audio_engine.hpp"
#include "core/logging.hpp"
#include <SDL.h>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace quin::audio {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    // Initialize SDL Audio subsystem if not already done
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            QUIN_LOG_WARN("AudioEngine: SDL_InitSubSystem(AUDIO) failed: {}. Running in silent mode.", SDL_GetError());
            m_initialized = true;
            return true;
        }
    }

    // Open SDL audio device with 48kHz stereo S16 format
    SDL_AudioSpec desired{};
    desired.freq = 48000;
    desired.format = AUDIO_S16LSB;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = nullptr; // Use SDL_QueueAudio push model

    SDL_AudioSpec obtained{};
    m_sdl_audio_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);

    if (m_sdl_audio_device_id == 0) {
        QUIN_LOG_WARN("AudioEngine: SDL_OpenAudioDevice failed: {}. Running in silent mode.", SDL_GetError());
    } else {
        m_device_sample_rate = obtained.freq;
        m_device_channels = obtained.channels;
        // Unpause the audio device so it starts playing queued audio
        SDL_PauseAudioDevice(m_sdl_audio_device_id, 0);
        QUIN_LOG_INFO("AudioEngine: SDL Audio Device opened — Rate: {} Hz | Channels: {} | Format: S16LE | Buffer: {} samples",
                      obtained.freq, obtained.channels, obtained.samples);
    }

    m_initialized = true;
    QUIN_LOG_INFO("AudioEngine Initialized — {} PCM Backend (48kHz Master).",
                  m_sdl_audio_device_id != 0 ? "SDL2 Real" : "Silent/Null");
    return true;
}

void AudioEngine::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    if (m_sdl_audio_device_id != 0) {
        SDL_CloseAudioDevice(m_sdl_audio_device_id);
        m_sdl_audio_device_id = 0;
        QUIN_LOG_INFO("AudioEngine: SDL Audio Device closed.");
    }

    m_ports.clear();
    m_initialized = false;
    QUIN_LOG_INFO("AudioEngine Shutdown cleanly.");
}

AudioPortHandle AudioEngine::open_port(const AudioOutPortConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    AudioPortHandle handle = m_next_handle++;

    AudioPortState state{};
    state.handle = handle;
    state.config = config;
    state.is_open = true;

    m_ports[handle] = state;
    QUIN_LOG_INFO("AudioEngine: Opened Audio Out Port #{} — Channels: {} | Rate: {} Hz",
                  handle, config.channel_count, config.sample_rate);
    return handle;
}

bool AudioEngine::close_port(AudioPortHandle handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ports.find(handle);
    if (it != m_ports.end()) {
        m_ports.erase(it);
        QUIN_LOG_INFO("AudioEngine: Closed Audio Out Port #{}", handle);
        return true;
    }
    return false;
}

bool AudioEngine::set_volume(AudioPortHandle handle, float left, float right) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ports.find(handle);
    if (it != m_ports.end()) {
        it->second.config.left_volume = left;
        it->second.config.right_volume = right;
        return true;
    }
    return false;
}

int64_t AudioEngine::submit_pcm_samples(AudioPortHandle handle, const void* pcm_data, size_t num_samples) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ports.find(handle);
    if (it == m_ports.end() || !it->second.is_open || !pcm_data) {
        return -1;
    }

    const auto& config = it->second.config;
    float left_vol = config.left_volume;
    float right_vol = config.right_volume;

    // Actually output audio via SDL if device is open
    if (m_sdl_audio_device_id != 0 && num_samples > 0) {
        if (config.format == AudioFormat::PCM_S16_LE) {
            // Direct S16 path — apply volume scaling
            size_t total_samples_count = num_samples * config.channel_count;
            std::vector<int16_t> scaled(total_samples_count);
            const int16_t* src = static_cast<const int16_t*>(pcm_data);

            for (size_t i = 0; i < total_samples_count; ++i) {
                float vol = (i % 2 == 0) ? left_vol : right_vol;
                float sample = static_cast<float>(src[i]) * vol;
                sample = std::clamp(sample, -32768.0f, 32767.0f);
                scaled[i] = static_cast<int16_t>(sample);
            }

            SDL_QueueAudio(m_sdl_audio_device_id, scaled.data(),
                           static_cast<uint32_t>(total_samples_count * sizeof(int16_t)));
        } else if (config.format == AudioFormat::PCM_F32_LE) {
            // Convert F32 → S16 with volume
            size_t total_samples_count = num_samples * config.channel_count;
            std::vector<int16_t> converted(total_samples_count);
            const float* src = static_cast<const float*>(pcm_data);

            for (size_t i = 0; i < total_samples_count; ++i) {
                float vol = (i % 2 == 0) ? left_vol : right_vol;
                float sample = src[i] * vol * 32767.0f;
                sample = std::clamp(sample, -32768.0f, 32767.0f);
                converted[i] = static_cast<int16_t>(sample);
            }

            SDL_QueueAudio(m_sdl_audio_device_id, converted.data(),
                           static_cast<uint32_t>(total_samples_count * sizeof(int16_t)));
        }
    }

    it->second.total_samples_submitted += num_samples;
    it->second.total_buffers_output++;
    m_total_samples_processed += num_samples;

    return static_cast<int64_t>(num_samples);
}

size_t AudioEngine::get_open_ports_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ports.size();
}

std::vector<AudioPortState> AudioEngine::get_active_ports() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AudioPortState> result;
    for (const auto& [handle, state] : m_ports) {
        result.push_back(state);
    }
    return result;
}

} // namespace quin::audio
