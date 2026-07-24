#ifndef QUIN_GPU_FRAME_PACING_HPP
#define QUIN_GPU_FRAME_PACING_HPP

#include <cstdint>
#include <mutex>

namespace quin::gpu {

enum class FramePacingMode : uint32_t {
    Locked30 = 30,
    Locked60 = 60,
    Unlocked = 0
};

class FramePacingRegulator {
public:
    FramePacingRegulator();

    void set_mode(FramePacingMode mode);
    void set_resolution_scale(float scale);

    void begin_frame();
    void end_frame();

    FramePacingMode get_mode() const { return m_mode; }
    float get_resolution_scale() const { return m_resolution_scale; }
    double get_last_frame_time_ms() const { return m_last_frame_time_ms; }
    double get_avg_frame_time_ms() const { return m_avg_frame_time_ms; }
    float get_current_fps() const;

private:
    FramePacingMode m_mode{FramePacingMode::Locked60};
    float m_resolution_scale{1.0f}; // 1.0 = Native 4K/1080p, 0.75 = FSR Quality
    double m_last_frame_time_ms{16.666};
    double m_avg_frame_time_ms{16.666};
    uint64_t m_frame_count{0};

    uint64_t m_frame_start_timestamp{0};
    mutable std::mutex m_mutex;
};

} // namespace quin::gpu

#endif // QUIN_GPU_FRAME_PACING_HPP
