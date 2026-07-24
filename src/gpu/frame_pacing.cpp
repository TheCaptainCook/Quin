#include "gpu/frame_pacing.hpp"
#include "core/logging.hpp"
#include <chrono>
#include <thread>
#include <algorithm>

namespace quin::gpu {

using Clock = std::chrono::high_resolution_clock;

FramePacingRegulator::FramePacingRegulator() = default;

void FramePacingRegulator::set_mode(FramePacingMode mode) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mode = mode;
    QUIN_LOG_INFO("FramePacingRegulator: Frame Pacing Mode set to {}",
                  mode == FramePacingMode::Locked30 ? "30 FPS Lock" :
                  (mode == FramePacingMode::Locked60 ? "60 FPS Lock" : "Unlocked"));
}

void FramePacingRegulator::set_resolution_scale(float scale) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resolution_scale = std::clamp(scale, 0.5f, 1.0f);
    QUIN_LOG_INFO("FramePacingRegulator: Dynamic Resolution Scale set to {:.2f}x", m_resolution_scale);
}

void FramePacingRegulator::begin_frame() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = Clock::now().time_since_epoch();
    m_frame_start_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

void FramePacingRegulator::end_frame() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = Clock::now().time_since_epoch();
    uint64_t end_ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

    double elapsed_ms = static_cast<double>(end_ts - m_frame_start_timestamp) / 1000.0;
    m_last_frame_time_ms = elapsed_ms;
    m_avg_frame_time_ms = m_avg_frame_time_ms * 0.9 + elapsed_ms * 0.1;
    m_frame_count++;

    if (m_mode != FramePacingMode::Unlocked) {
        double target_ms = 1000.0 / static_cast<double>(m_mode);
        if (elapsed_ms < target_ms) {
            double sleep_ms = target_ms - elapsed_ms;
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<uint64_t>(sleep_ms * 1000.0)));
        }
    }
}

float FramePacingRegulator::get_current_fps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_avg_frame_time_ms <= 0.0) return 60.0f;
    return static_cast<float>(1000.0 / m_avg_frame_time_ms);
}

} // namespace quin::gpu
