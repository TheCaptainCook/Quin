#ifndef QUIN_COMPAT_TITLE_DB_HPP
#define QUIN_COMPAT_TITLE_DB_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace quin::compat {

enum class CompatStatus : uint32_t {
    Nothing = 0,
    Boots,
    Menu,
    Ingame,
    Playable,
    Perfect
};

struct TitleEntry {
    std::string title_id; // e.g. "CUSA00001"
    std::string name;
    std::string region;   // e.g. "US", "EU", "JP"
    CompatStatus status{CompatStatus::Nothing};
    std::vector<std::string> missing_symbols;
    std::string last_tested_date;
    uint32_t target_fps{60};
};

class TitleDatabase {
public:
    TitleDatabase();

    void add_or_update_title(const TitleEntry& entry);
    bool get_title(const std::string& title_id, TitleEntry& out_entry) const;
    std::vector<TitleEntry> get_all_titles() const;

    size_t get_count_by_status(CompatStatus status) const;
    std::string export_to_markdown() const;

private:
    void populate_default_database();

    std::unordered_map<std::string, TitleEntry> m_titles;
    mutable std::mutex m_mutex;
};

std::string compat_status_to_string(CompatStatus status);

} // namespace quin::compat

#endif // QUIN_COMPAT_TITLE_DB_HPP
