#include "gpu/shader/shader_cache.hpp"
#include "core/logging.hpp"

namespace quin::gpu::shader {

bool ShaderCache::contains(ShaderHash hash) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.find(hash) != m_cache.end();
}

const CompiledShader* ShaderCache::get(ShaderHash hash) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(hash);
    if (it != m_cache.end()) {
        m_cache_hits++;
        return &it->second;
    }
    m_cache_misses++;
    return nullptr;
}

void ShaderCache::put(const CompiledShader& shader) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[shader.hash] = shader;
    QUIN_LOG_INFO("ShaderCache: Cached Compiled SPIR-V Shader — Hash: 0x{:016X}", shader.hash);
}

size_t ShaderCache::get_cached_shader_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.size();
}

std::vector<CompiledShader> ShaderCache::get_all_cached_shaders() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<CompiledShader> result;
    for (const auto& [hash, shader] : m_cache) {
        result.push_back(shader);
    }
    return result;
}

} // namespace quin::gpu::shader
