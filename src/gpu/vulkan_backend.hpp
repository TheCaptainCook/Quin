#ifndef QUIN_GPU_VULKAN_BACKEND_HPP
#define QUIN_GPU_VULKAN_BACKEND_HPP

#include "gpu/gnm_types.hpp"
#include "gpu/resource_translator.hpp"
#include "gpu/gnm_parser.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace quin::gpu {

struct VulkanDeviceInfo {
    std::string device_name{"AMD Radeon RX 6700 XT (RDNA2 / Vulkan 1.3)"};
    std::string driver_version{"24.5.1"};
    uint32_t api_version{13}; // Vulkan 1.3
    uint64_t vram_bytes{16ULL * 1024 * 1024 * 1024}; // 16GB
    bool initialized{false};
};

struct DummyPipeline {
    PsoKey key;
    uint64_t pipeline_handle{0};
    uint64_t cache_hit_count{0};
};

class VulkanBackend {
public:
    VulkanBackend();
    ~VulkanBackend();

    bool initialize();
    void shutdown();

    uint64_t get_or_create_pipeline(const PsoKey& key);
    void submit_command_buffer(const ParseResult& parse_result);

    const VulkanDeviceInfo& get_device_info() const { return m_device_info; }
    size_t get_cached_pipelines_count() const;
    uint64_t get_total_pso_cache_hits() const { return m_total_pso_cache_hits; }
    uint64_t get_total_draw_calls_rendered() const { return m_total_draw_calls_rendered; }

private:
    VulkanDeviceInfo m_device_info;
    std::unordered_map<PsoKey, DummyPipeline, PsoKeyHash> m_pso_cache;
    uint64_t m_next_pipeline_id{1001};
    uint64_t m_total_pso_cache_hits{0};
    uint64_t m_total_draw_calls_rendered{0};
    mutable std::mutex m_mutex;
};

} // namespace quin::gpu

#endif // QUIN_GPU_VULKAN_BACKEND_HPP
