#ifndef QUIN_GUI_IMGUI_SINK_HPP
#define QUIN_GUI_IMGUI_SINK_HPP

#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>

namespace quin::gui {

struct LogEntry {
    spdlog::level::level_enum level;
    std::string time_str;
    std::string payload;
};

template<typename Mutex>
class ImGuiLogSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit ImGuiLogSink(size_t max_entries = 2000)
        : m_max_entries(max_entries) {}

    std::vector<LogEntry> get_entries() const {
        std::lock_guard<Mutex> lock(m_sink_mutex);
        return m_entries;
    }

    void clear() {
        std::lock_guard<Mutex> lock(m_sink_mutex);
        m_entries.clear();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm tm_buf{};
#if defined(_WIN32)
        localtime_s(&tm_buf, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_buf);
#endif
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_buf);

        LogEntry entry{
            msg.level,
            std::string(time_str),
            fmt::to_string(msg.payload)
        };

        std::lock_guard<Mutex> lock(m_sink_mutex);
        if (m_entries.size() >= m_max_entries) {
            m_entries.erase(m_entries.begin());
        }
        m_entries.push_back(entry);
    }

    void flush_() override {}

private:
    size_t m_max_entries;
    mutable Mutex m_sink_mutex;
    std::vector<LogEntry> m_entries;
};

using ImGuiLogSink_mt = ImGuiLogSink<std::mutex>;

} // namespace quin::gui

#endif // QUIN_GUI_IMGUI_SINK_HPP
