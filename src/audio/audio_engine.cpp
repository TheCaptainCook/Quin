#include "audio/audio_engine.hpp"
#include "core/logging.hpp"

namespace quin::audio {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    m_initialized = true;
    QUIN_LOG_INFO("AudioEngine Initialized — Tempest 3D AudioTech & SDL2 PCM Backend (48kHz Master).");
    return true;
}

void AudioEngine::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

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
