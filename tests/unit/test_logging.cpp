#include <catch2/catch_test_macros.hpp>
#include "core/logging.hpp"

TEST_CASE("Logging System Initialization and Log Interception", "[core][logging]") {
    // 1. Initialize logging
    quin::core::init_logging();

    auto logger = quin::core::get_logger();
    REQUIRE(logger != nullptr);

    auto sink = quin::core::get_imgui_sink();
    REQUIRE(sink != nullptr);

    // Clear any previous logs
    sink->clear();
    REQUIRE(sink->get_entries().empty());

    // 2. Emit test log entries across different levels
    QUIN_LOG_INFO("Test Info Message: {}", 42);
    QUIN_LOG_WARN("Test Warning Message: {}", "WarningPayload");
    QUIN_LOG_ERROR("Test Error Message");

    // 3. Verify logs captured by ImGui custom sink
    auto entries = sink->get_entries();
    REQUIRE(entries.size() == 3);

    REQUIRE(entries[0].level == spdlog::level::info);
    REQUIRE(entries[0].payload == "Test Info Message: 42");

    REQUIRE(entries[1].level == spdlog::level::warn);
    REQUIRE(entries[1].payload == "Test Warning Message: WarningPayload");

    REQUIRE(entries[2].level == spdlog::level::err);
    REQUIRE(entries[2].payload == "Test Error Message");

    // 4. Test sink clearing
    sink->clear();
    REQUIRE(sink->get_entries().empty());
}
