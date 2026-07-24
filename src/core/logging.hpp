#ifndef QUIN_CORE_LOGGING_HPP
#define QUIN_CORE_LOGGING_HPP

#include <spdlog/spdlog.h>
#include <memory>
#include "gui/imgui_sink.hpp"

namespace quin::core {

void init_logging();
std::shared_ptr<spdlog::logger> get_logger();
std::shared_ptr<quin::gui::ImGuiLogSink_mt> get_imgui_sink();

} // namespace quin::core

#define QUIN_LOG_TRACE(...)    spdlog::trace(__VA_ARGS__)
#define QUIN_LOG_DEBUG(...)    spdlog::debug(__VA_ARGS__)
#define QUIN_LOG_INFO(...)     spdlog::info(__VA_ARGS__)
#define QUIN_LOG_WARN(...)     spdlog::warn(__VA_ARGS__)
#define QUIN_LOG_ERROR(...)    spdlog::error(__VA_ARGS__)
#define QUIN_LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)

#endif // QUIN_CORE_LOGGING_HPP
