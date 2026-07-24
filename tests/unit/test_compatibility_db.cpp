#include <catch2/catch_test_macros.hpp>
#include "compat/title_db.hpp"
#include "compat/compat_triage.hpp"

TEST_CASE("Per-Title Compatibility Database & Markdown Matrix", "[compat][db]") {
    quin::compat::TitleDatabase db;

    // 1. Initial count checks
    REQUIRE(db.get_all_titles().size() >= 5);
    REQUIRE(db.get_count_by_status(quin::compat::CompatStatus::Playable) >= 2);

    // 2. Add custom test title
    quin::compat::TitleEntry custom{};
    custom.title_id = "CUSA99999";
    custom.name = "Custom Unit Test Game";
    custom.region = "US";
    custom.status = quin::compat::CompatStatus::Playable;
    custom.target_fps = 60;
    custom.last_tested_date = "2026-07-24";

    db.add_or_update_title(custom);

    quin::compat::TitleEntry retrieved{};
    REQUIRE(db.get_title("CUSA99999", retrieved) == true);
    REQUIRE(retrieved.name == "Custom Unit Test Game");
    REQUIRE(retrieved.status == quin::compat::CompatStatus::Playable);

    // 3. Export Markdown matrix
    std::string md = db.export_to_markdown();
    REQUIRE(md.find("CUSA99999") != std::string::npos);
    REQUIRE(md.find("Custom Unit Test Game") != std::string::npos);
}

TEST_CASE("Automated Symbol Triage & Regression Runner", "[compat][triage]") {
    quin::compat::CompatTriage triage;

    // 1. Log missing symbols
    triage.log_missing_symbol("sceGnmSubmitDone", "libSceGnmDriver");
    triage.log_missing_symbol("sceGnmSubmitDone", "libSceGnmDriver");
    triage.log_missing_symbol("sceAudioOutGetPortState", "libSceAudioOut");

    REQUIRE(triage.get_total_unimplemented_calls() == 3);

    auto top = triage.get_top_missing_symbols(10);
    REQUIRE(top.size() >= 2);
    REQUIRE(top[0].symbol_name == "sceGnmSubmitDone");
    REQUIRE(top[0].call_count == 2);

    // 2. Run Automated Regression Suite
    auto reg_result = triage.run_regression_suite();
    REQUIRE(reg_result.all_passed == true);
    REQUIRE(reg_result.tests_passed == 4);
    REQUIRE(reg_result.tests_failed == 0);
}
