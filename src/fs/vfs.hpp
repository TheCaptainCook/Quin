#ifndef QUIN_FS_VFS_HPP
#define QUIN_FS_VFS_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <mutex>

namespace quin::fs {

using VfsFileHandle = int32_t;
constexpr VfsFileHandle INVALID_VFS_HANDLE = -1;

struct MountPoint {
    std::string virtual_prefix;
    std::string host_directory;
};

struct FileInfo {
    VfsFileHandle handle{INVALID_VFS_HANDLE};
    std::string virtual_path;
    std::string host_path;
    uint64_t size{0};
    uint64_t position{0};
    bool is_open{false};
};

class VirtualFileSystem {
public:
    VirtualFileSystem();
    ~VirtualFileSystem();

    bool mount(const std::string& virtual_prefix, const std::string& host_directory);
    bool unmount(const std::string& virtual_prefix);

    VfsFileHandle open_file(const std::string& guest_vpath, uint32_t flags = 0);
    int64_t read_file(VfsFileHandle handle, void* dest, size_t size);
    int64_t write_file(VfsFileHandle handle, const void* src, size_t size);
    bool close_file(VfsFileHandle handle);
    int64_t seek_file(VfsFileHandle handle, int64_t offset, int whence);
    bool stat_file(const std::string& guest_vpath, uint64_t& out_size) const;

    std::string resolve_path(const std::string& guest_vpath) const;
    std::vector<MountPoint> get_mount_points() const;
    std::vector<FileInfo> get_open_files() const;

    uint64_t get_total_read_bytes() const { return m_total_read_bytes; }
    uint64_t get_total_written_bytes() const { return m_total_written_bytes; }

private:
    std::vector<MountPoint> m_mounts;
    std::unordered_map<VfsFileHandle, FileInfo> m_open_files;
    std::unordered_map<VfsFileHandle, std::shared_ptr<std::fstream>> m_streams;

    VfsFileHandle m_next_handle{100};
    uint64_t m_total_read_bytes{0};
    uint64_t m_total_written_bytes{0};
    mutable std::mutex m_mutex;
};

} // namespace quin::fs

#endif // QUIN_FS_VFS_HPP
