#include "fs/vfs.hpp"
#include "core/logging.hpp"
#include <algorithm>
#include <filesystem>

namespace quin::fs {

namespace fs_sys = std::filesystem;

VirtualFileSystem::VirtualFileSystem() {
    // Mount default system directories
    mount("/app0/", "./games/default_app/");
    mount("/data/", "./data/");
    mount("/system/", "./system/");
    mount("/savedata/", "./savedata/");
}

VirtualFileSystem::~VirtualFileSystem() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [handle, stream] : m_streams) {
        if (stream && stream->is_open()) {
            stream->close();
        }
    }
    m_streams.clear();
    m_open_files.clear();
}

bool VirtualFileSystem::mount(const std::string& virtual_prefix, const std::string& host_directory) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Normalize prefix trailing slash
    std::string prefix = virtual_prefix;
    if (!prefix.empty() && prefix.back() != '/') prefix += '/';

    // Ensure host directory exists
    try {
        fs_sys::create_directories(host_directory);
    } catch (...) {}

    // Check if already mounted
    for (auto& m : m_mounts) {
        if (m.virtual_prefix == prefix) {
            m.host_directory = host_directory;
            QUIN_LOG_INFO("VFS Remounted — Prefix: '{}' -> Host: '{}'", prefix, host_directory);
            return true;
        }
    }

    m_mounts.push_back(MountPoint{prefix, host_directory});
    QUIN_LOG_INFO("VFS Mounted — Prefix: '{}' -> Host: '{}'", prefix, host_directory);
    return true;
}

bool VirtualFileSystem::unmount(const std::string& virtual_prefix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string prefix = virtual_prefix;
    if (!prefix.empty() && prefix.back() != '/') prefix += '/';

    auto it = std::find_if(m_mounts.begin(), m_mounts.end(), [&prefix](const MountPoint& m) {
        return m.virtual_prefix == prefix;
    });

    if (it != m_mounts.end()) {
        m_mounts.erase(it);
        QUIN_LOG_INFO("VFS Unmounted — Prefix: '{}'", prefix);
        return true;
    }
    return false;
}

std::string VirtualFileSystem::resolve_path(const std::string& guest_vpath) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string path = guest_vpath;
    // Standardize leading slash
    if (path.empty() || path[0] != '/') path = "/" + path;

    for (const auto& mount : m_mounts) {
        if (path.rfind(mount.virtual_prefix, 0) == 0) {
            std::string relative = path.substr(mount.virtual_prefix.length());
            fs_sys::path host_p = fs_sys::path(mount.host_directory) / relative;
            return host_p.string();
        }
    }

    // Default fallback to root relative
    return (fs_sys::path("./") / path.substr(1)).string();
}

VfsFileHandle VirtualFileSystem::open_file(const std::string& guest_vpath, uint32_t /*flags*/) {
    std::string host_p = resolve_path(guest_vpath);

    // Create parent directories if they don't exist
    try {
        fs_sys::path parent_dir = fs_sys::path(host_p).parent_path();
        if (!parent_dir.empty()) {
            fs_sys::create_directories(parent_dir);
        }
    } catch (...) {}

    std::lock_guard<std::mutex> lock(m_mutex);
    VfsFileHandle handle = m_next_handle++;

    auto stream = std::make_shared<std::fstream>();
    stream->open(host_p, std::ios::in | std::ios::out | std::ios::binary);

    if (!stream->is_open()) {
        // Try opening read-only if read-write failed
        stream->open(host_p, std::ios::in | std::ios::binary);
    }

    if (!stream->is_open()) {
        // Create new file if it doesn't exist
        stream->open(host_p, std::ios::out | std::ios::binary);
        stream->close();
        stream->open(host_p, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!stream->is_open()) {
        QUIN_LOG_WARN("VFS open_file failed for Guest Path: '{}' (Host: '{}')", guest_vpath, host_p);
        return INVALID_VFS_HANDLE;
    }

    stream->seekg(0, std::ios::end);
    uint64_t file_sz = stream->tellg();
    stream->seekg(0, std::ios::beg);

    FileInfo info{
        handle,
        guest_vpath,
        host_p,
        file_sz,
        0,
        true
    };

    m_open_files[handle] = info;
    m_streams[handle] = stream;

    QUIN_LOG_INFO("VFS File Opened — FD: {} | Guest: '{}' | Size: {} bytes", handle, guest_vpath, file_sz);
    return handle;
}

int64_t VirtualFileSystem::read_file(VfsFileHandle handle, void* dest, size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto st_it = m_streams.find(handle);
    if (st_it == m_streams.end() || !st_it->second || !st_it->second->is_open()) {
        return -1;
    }

    st_it->second->read(static_cast<char*>(dest), size);
    std::streamsize bytes_read = st_it->second->gcount();

    m_open_files[handle].position += bytes_read;
    m_total_read_bytes += bytes_read;

    return static_cast<int64_t>(bytes_read);
}

int64_t VirtualFileSystem::write_file(VfsFileHandle handle, const void* src, size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto st_it = m_streams.find(handle);
    if (st_it == m_streams.end() || !st_it->second || !st_it->second->is_open()) {
        return -1;
    }

    st_it->second->write(static_cast<const char*>(src), size);
    st_it->second->flush();

    m_open_files[handle].position += size;
    m_open_files[handle].size = std::max<uint64_t>(m_open_files[handle].size, m_open_files[handle].position);
    m_total_written_bytes += size;

    return static_cast<int64_t>(size);
}

bool VirtualFileSystem::close_file(VfsFileHandle handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto st_it = m_streams.find(handle);
    if (st_it != m_streams.end()) {
        if (st_it->second && st_it->second->is_open()) {
            st_it->second->close();
        }
        m_streams.erase(st_it);
        m_open_files.erase(handle);
        QUIN_LOG_INFO("VFS File Closed — FD: {}", handle);
        return true;
    }
    return false;
}

int64_t VirtualFileSystem::seek_file(VfsFileHandle handle, int64_t offset, int whence) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto st_it = m_streams.find(handle);
    if (st_it == m_streams.end() || !st_it->second || !st_it->second->is_open()) {
        return -1;
    }

    std::ios_base::seekdir dir;
    switch (whence) {
        case 0: dir = std::ios::beg; break;  // SEEK_SET
        case 1: dir = std::ios::cur; break;  // SEEK_CUR
        case 2: dir = std::ios::end; break;  // SEEK_END
        default: return -1;
    }

    st_it->second->seekg(offset, dir);
    st_it->second->seekp(offset, dir);

    auto new_pos = st_it->second->tellg();
    if (new_pos >= 0) {
        m_open_files[handle].position = static_cast<uint64_t>(new_pos);
    }

    return static_cast<int64_t>(new_pos);
}

bool VirtualFileSystem::stat_file(const std::string& guest_vpath, uint64_t& out_size) const {
    std::string host_p = resolve_path(guest_vpath);
    try {
        if (fs_sys::exists(host_p)) {
            out_size = fs_sys::file_size(host_p);
            return true;
        }
    } catch (...) {}
    out_size = 0;
    return false;
}

std::vector<MountPoint> VirtualFileSystem::get_mount_points() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mounts;
}

std::vector<FileInfo> VirtualFileSystem::get_open_files() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileInfo> result;
    for (const auto& [fd, info] : m_open_files) {
        result.push_back(info);
    }
    return result;
}

} // namespace quin::fs
