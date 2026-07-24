#include "core/logging.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

namespace quin::core {

static std::shared_ptr<spdlog::logger> s_logger;
static std::shared_ptr<quin::gui::ImGuiLogSink_mt> s_imgui_sink;

void init_logging() {
    if (s_logger) return;

    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    s_imgui_sink = std::make_shared<quin::gui::ImGuiLogSink_mt>();

    std::vector<spdlog::sink_ptr> sinks { stdout_sink, s_imgui_sink };
    s_logger = std::make_shared<spdlog::logger>("quin", sinks.begin(), sinks.end());
    s_logger->set_level(spdlog::level::trace);
    s_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    spdlog::set_default_logger(s_logger);
    spdlog::set_level(spdlog::level::trace);

    QUIN_LOG_INFO("Quin Logging System initialized.");
}

std::shared_ptr<spdlog::logger> get_logger() {
    return s_logger;
}

std::shared_ptr<quin::gui::ImGuiLogSink_mt> get_imgui_sink() {
    return s_imgui_sink;
}

} // namespace quin::core
