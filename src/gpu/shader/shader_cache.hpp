#ifndef QUIN_GPU_SHADER_SHADER_CACHE_HPP
#define QUIN_GPU_SHADER_SHADER_CACHE_HPP

#include "gpu/shader/shader_types.hpp"
#include <unordered_map>
#include <mutex>
#include <vector>

namespace quin::gpu::shader {

class ShaderCache {
public:
    ShaderCache() = default;

    bool contains(ShaderHash hash) const;
    const CompiledShader* get(ShaderHash hash) const;
    void put(const CompiledShader& shader);

    size_t get_cached_shader_count() const;
    uint64_t get_cache_hits() const { return m_cache_hits; }
    uint64_t get_cache_misses() const { return m_cache_misses; }

    std::vector<CompiledShader> get_all_cached_shaders() const;

private:
    std::unordered_map<ShaderHash, CompiledShader> m_cache;
    mutable uint64_t m_cache_hits{0};
    mutable uint64_t m_cache_misses{0};
    mutable std::mutex m_mutex;
};

} // namespace quin::gpu::shader

#endif // QUIN_GPU_SHADER_SHADER_CACHE_HPP
