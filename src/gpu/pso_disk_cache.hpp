#ifndef QUIN_GPU_PSO_DISK_CACHE_HPP
#define QUIN_GPU_PSO_DISK_CACHE_HPP

#include "gpu/resource_translator.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace quin::gpu {

struct DiskPsoRecord {
    PsoKey key;
    uint64_t pipeline_id{0};
    uint64_t compiled_timestamp{0};
};

class PsoDiskCache {
public:
    explicit PsoDiskCache(std::string cache_directory = "./cache/");

    bool load_from_disk();
    bool save_to_disk();

    void put_record(const PsoKey& key, uint64_t pipeline_id);
    bool get_record(const PsoKey& key, DiskPsoRecord& out_record) const;

    size_t get_records_count() const;
    const std::string& get_cache_dir() const { return m_cache_directory; }

private:
    std::string m_cache_directory;
    std::unordered_map<PsoKey, DiskPsoRecord, PsoKeyHash> m_disk_records;
    mutable std::mutex m_mutex;
};

} // namespace quin::gpu

#endif // QUIN_GPU_PSO_DISK_CACHE_HPP
