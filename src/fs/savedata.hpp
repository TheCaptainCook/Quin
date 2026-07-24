#ifndef QUIN_FS_SAVEDATA_HPP
#define QUIN_FS_SAVEDATA_HPP

#include "fs/vfs.hpp"
#include <string>
#include <cstdint>

namespace quin::fs {

struct SaveDataConfig {
    uint32_t user_id{1000};
    std::string title_id{"CUSA00000"};
    uint64_t quota_bytes{32 * 1024 * 1024}; // 32MB default
};

class SaveDataManager {
public:
    explicit SaveDataManager(VirtualFileSystem& vfs);

    bool mount_savedata(const SaveDataConfig& config);
    bool unmount_savedata(uint32_t user_id, const std::string& title_id);

    std::string get_save_directory(uint32_t user_id, const std::string& title_id) const;
    bool has_savedata(uint32_t user_id, const std::string& title_id, const std::string& filename) const;

private:
    VirtualFileSystem& m_vfs;
};

} // namespace quin::fs

#endif // QUIN_FS_SAVEDATA_HPP
