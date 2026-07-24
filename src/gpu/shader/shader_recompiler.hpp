#ifndef QUIN_GPU_SHADER_SHADER_RECOMPILER_HPP
#define QUIN_GPU_SHADER_SHADER_RECOMPILER_HPP

#include "gpu/shader/shader_types.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace quin::gpu::shader {

class ShaderRecompiler {
public:
    ShaderRecompiler() = default;

    ShaderRecompileResult recompile(const void* rdna2_bytes, size_t size_bytes, ShaderType stage);
    static ShaderHash compute_hash(const void* bytes, size_t size);

    uint64_t get_total_shaders_compiled() const { return m_total_compiled; }

private:
    std::vector<uint32_t> emit_spirv_header(ShaderType stage);

    uint64_t m_total_compiled{0};
};

} // namespace quin::gpu::shader

#endif // QUIN_GPU_SHADER_SHADER_RECOMPILER_HPP
