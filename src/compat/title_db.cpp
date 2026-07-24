#include "compat/title_db.hpp"
#include "core/logging.hpp"
#include <sstream>
#include <iomanip>

namespace quin::compat {

std::string compat_status_to_string(CompatStatus status) {
    switch (status) {
        case CompatStatus::Nothing:  return "Nothing";
        case CompatStatus::Boots:    return "Boots";
        case CompatStatus::Menu:     return "Menu";
        case CompatStatus::Ingame:   return "In-game";
        case CompatStatus::Playable: return "Playable";
        case CompatStatus::Perfect:  return "Perfect";
        default:                     return "Unknown";
    }
}

TitleDatabase::TitleDatabase() {
    populate_default_database();
}

void TitleDatabase::populate_default_database() {
    add_or_update_title(TitleEntry{
        "CUSA00001", "Astro's Playroom Demo Target", "US", CompatStatus::Playable, {}, "2026-07-24", 60
    });
    add_or_update_title(TitleEntry{
        "CUSA00002", "Homebrew Sample Suite", "GLOBAL", CompatStatus::Perfect, {}, "2026-07-24", 60
    });
    add_or_update_title(TitleEntry{
        "CUSA00003", "2D Sprite Test Target", "US", CompatStatus::Playable, {}, "2026-07-24", 60
    });
    add_or_update_title(TitleEntry{
        "CUSA00004", "3D Cube Shader Benchmark", "EU", CompatStatus::Ingame, {"sceGnmSubmitDone"}, "2026-07-24", 60
    });
    add_or_update_title(TitleEntry{
        "CUSA00005", "Audio Tempest 3D Test", "JP", CompatStatus::Boots, {"sceAudioOutGetPortState"}, "2026-07-24", 60
    });
}

void TitleDatabase::add_or_update_title(const TitleEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_titles[entry.title_id] = entry;
    QUIN_LOG_INFO("TitleDatabase: Registered Title '{}' [{}] — Status: {}",
                  entry.name, entry.title_id, compat_status_to_string(entry.status));
}

bool TitleDatabase::get_title(const std::string& title_id, TitleEntry& out_entry) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_titles.find(title_id);
    if (it != m_titles.end()) {
        out_entry = it->second;
        return true;
    }
    return false;
}

std::vector<TitleEntry> TitleDatabase::get_all_titles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<TitleEntry> result;
    for (const auto& [id, entry] : m_titles) {
        result.push_back(entry);
    }
    return result;
}

size_t TitleDatabase::get_count_by_status(CompatStatus status) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& [id, entry] : m_titles) {
        if (entry.status == status) count++;
    }
    return count;
}

std::string TitleDatabase::export_to_markdown() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;

    ss << "# Quin PS5 Emulator — Compatibility Matrix\n\n";
    ss << "| Title ID | Title Name | Region | Status | Target FPS | Last Tested |\n";
    ss << "| :--- | :--- | :---: | :---: | :---: | :---: |\n";

    for (const auto& [id, entry] : m_titles) {
        ss << "| `" << entry.title_id << "` | " << entry.name << " | " << entry.region
           << " | **" << compat_status_to_string(entry.status) << "** | "
           << entry.target_fps << " FPS | " << entry.last_tested_date << " |\n";
    }

    return ss.str();
}

} // namespace quin::compat
