#include "fs/savedata.hpp"
#include "core/logging.hpp"
#include <filesystem>

namespace quin::fs {

namespace fs_sys = std::filesystem;

SaveDataManager::SaveDataManager(VirtualFileSystem& vfs)
    : m_vfs(vfs) {}

bool SaveDataManager::mount_savedata(const SaveDataConfig& config) {
    std::string dir = get_save_directory(config.user_id, config.title_id);
    try {
        fs_sys::create_directories(dir);
    } catch (...) {}

    std::string vprefix = "/savedata/" + std::to_string(config.user_id) + "/" + config.title_id + "/";
    bool ok = m_vfs.mount(vprefix, dir);

    QUIN_LOG_INFO("SaveData Mounted — UserID: {} | TitleID: '{}' | HostDir: '{}'",
                  config.user_id, config.title_id, dir);
    return ok;
}

bool SaveDataManager::unmount_savedata(uint32_t user_id, const std::string& title_id) {
    std::string vprefix = "/savedata/" + std::to_string(user_id) + "/" + title_id + "/";
    return m_vfs.unmount(vprefix);
}

std::string SaveDataManager::get_save_directory(uint32_t user_id, const std::string& title_id) const {
    return "./savedata/" + std::to_string(user_id) + "/" + title_id + "/";
}

bool SaveDataManager::has_savedata(uint32_t user_id, const std::string& title_id, const std::string& filename) const {
    std::string host_path = get_save_directory(user_id, title_id) + filename;
    return fs_sys::exists(host_path);
}

} // namespace quin::fs
