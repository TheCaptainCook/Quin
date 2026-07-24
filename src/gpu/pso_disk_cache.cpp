#include "gpu/pso_disk_cache.hpp"
#include "core/logging.hpp"
#include <filesystem>
#include <fstream>

namespace quin::gpu {

namespace fs_sys = std::filesystem;

PsoDiskCache::PsoDiskCache(std::string cache_directory)
    : m_cache_directory(std::move(cache_directory)) {
    try {
        fs_sys::create_directories(m_cache_directory);
    } catch (...) {}
}

bool PsoDiskCache::load_from_disk() {
    std::lock_guard<std::mutex> lock(m_mutex);
    fs_sys::path cache_file = fs_sys::path(m_cache_directory) / "pso_cache.bin";

    if (!fs_sys::exists(cache_file)) {
        return false;
    }

    std::ifstream stream(cache_file, std::ios::binary);
    if (!stream.is_open()) return false;

    uint32_t magic = 0;
    stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x5150534F) { // 'QPSO'
        return false;
    }

    uint32_t count = 0;
    stream.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (uint32_t i = 0; i < count; ++i) {
        DiskPsoRecord rec{};
        stream.read(reinterpret_cast<char*>(&rec.key.rt_format), sizeof(rec.key.rt_format));
        stream.read(reinterpret_cast<char*>(&rec.key.depth_format), sizeof(rec.key.depth_format));
        stream.read(reinterpret_cast<char*>(&rec.key.primitive_type), sizeof(rec.key.primitive_type));
        stream.read(reinterpret_cast<char*>(&rec.key.depth_test), sizeof(rec.key.depth_test));
        stream.read(reinterpret_cast<char*>(&rec.key.blend_enable), sizeof(rec.key.blend_enable));
        stream.read(reinterpret_cast<char*>(&rec.pipeline_id), sizeof(rec.pipeline_id));
        stream.read(reinterpret_cast<char*>(&rec.compiled_timestamp), sizeof(rec.compiled_timestamp));

        m_disk_records[rec.key] = rec;
    }

    QUIN_LOG_INFO("PsoDiskCache: Loaded {} persistent PSOs from disk cache.", count);
    return true;
}

bool PsoDiskCache::save_to_disk() {
    std::lock_guard<std::mutex> lock(m_mutex);
    fs_sys::path cache_file = fs_sys::path(m_cache_directory) / "pso_cache.bin";

    std::ofstream stream(cache_file, std::ios::binary);
    if (!stream.is_open()) return false;

    uint32_t magic = 0x5150534F; // 'QPSO'
    stream.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    uint32_t count = static_cast<uint32_t>(m_disk_records.size());
    stream.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& [key, rec] : m_disk_records) {
        stream.write(reinterpret_cast<const char*>(&rec.key.rt_format), sizeof(rec.key.rt_format));
        stream.write(reinterpret_cast<const char*>(&rec.key.depth_format), sizeof(rec.key.depth_format));
        stream.write(reinterpret_cast<const char*>(&rec.key.primitive_type), sizeof(rec.key.primitive_type));
        stream.write(reinterpret_cast<const char*>(&rec.key.depth_test), sizeof(rec.key.depth_test));
        stream.write(reinterpret_cast<const char*>(&rec.key.blend_enable), sizeof(rec.key.blend_enable));
        stream.write(reinterpret_cast<const char*>(&rec.pipeline_id), sizeof(rec.pipeline_id));
        stream.write(reinterpret_cast<const char*>(&rec.compiled_timestamp), sizeof(rec.compiled_timestamp));
    }

    QUIN_LOG_INFO("PsoDiskCache: Saved {} persistent PSOs to disk cache.", count);
    return true;
}

void PsoDiskCache::put_record(const PsoKey& key, uint64_t pipeline_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    DiskPsoRecord rec{key, pipeline_id, 100000000ULL};
    m_disk_records[key] = rec;
}

bool PsoDiskCache::get_record(const PsoKey& key, DiskPsoRecord& out_record) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_disk_records.find(key);
    if (it != m_disk_records.end()) {
        out_record = it->second;
        return true;
    }
    return false;
}

size_t PsoDiskCache::get_records_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_disk_records.size();
}

} // namespace quin::gpu
