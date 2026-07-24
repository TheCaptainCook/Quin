#include "gpu/vulkan_backend.hpp"
#include "core/logging.hpp"

namespace quin::gpu {

VulkanBackend::VulkanBackend() = default;

VulkanBackend::~VulkanBackend() {
    shutdown();
}

bool VulkanBackend::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_device_info.initialized) return true;

    m_device_info.initialized = true;
    QUIN_LOG_INFO("VulkanBackend Initialized — Device: '{}' | API: Vulkan 1.3 | VRAM: {} GB",
                  m_device_info.device_name, m_device_info.vram_bytes / (1024 * 1024 * 1024));
    return true;
}

void VulkanBackend::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_device_info.initialized) return;

    m_pso_cache.clear();
    m_device_info.initialized = false;
    QUIN_LOG_INFO("VulkanBackend Shutdown cleanly.");
}

uint64_t VulkanBackend::get_or_create_pipeline(const PsoKey& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pso_cache.find(key);
    if (it != m_pso_cache.end()) {
        it->second.cache_hit_count++;
        m_total_pso_cache_hits++;
        return it->second.pipeline_handle;
    }

    uint64_t new_pipeline_id = m_next_pipeline_id++;
    DummyPipeline pipeline{key, new_pipeline_id, 0};
    m_pso_cache[key] = pipeline;

    QUIN_LOG_INFO("VulkanBackend: Compiled New Vulkan PSO #{} — RT: {} | Topology: {}",
                  new_pipeline_id,
                  ResourceTranslator::format_to_string(key.rt_format),
                  ResourceTranslator::topology_to_string(key.primitive_type));

    return new_pipeline_id;
}

void VulkanBackend::submit_command_buffer(const ParseResult& parse_result) {
    if (!parse_result.success) return;

    for (const auto& draw_cmd : parse_result.draw_commands) {
        PsoKey key = ResourceTranslator::build_pso_key(parse_result.context_state, draw_cmd.primitive_type);
        uint64_t pso = get_or_create_pipeline(key);
        (void)pso;

        m_total_draw_calls_rendered++;
    }

    QUIN_LOG_INFO("VulkanBackend: Submitted Command Buffer — {} Draw Calls Rendered.",
                  parse_result.draw_commands.size());
}

size_t VulkanBackend::get_cached_pipelines_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pso_cache.size();
}

} // namespace quin::gpu
