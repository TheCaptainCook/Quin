#ifndef QUIN_GPU_SHADER_SHADER_TYPES_HPP
#define QUIN_GPU_SHADER_SHADER_TYPES_HPP

#include <cstdint>
#include <vector>
#include <string>

namespace quin::gpu::shader {

constexpr uint32_t SPIRV_MAGIC_NUMBER = 0x07230203;
constexpr uint32_t SPIRV_VERSION      = 0x00010500;

enum class ShaderType : uint32_t {
    Vertex = 0,
    Pixel,
    Compute
};

using ShaderHash = uint64_t;

struct CompiledShader {
    ShaderHash hash{0};
    ShaderType stage{ShaderType::Vertex};
    std::vector<uint32_t> spirv_code;
    std::vector<uint8_t> rdna2_bytecode;
    bool is_valid{false};
};

struct ShaderRecompileResult {
    bool success{false};
    CompiledShader shader;
    std::string error_message;
    size_t instructions_recompiled{0};
};

} // namespace quin::gpu::shader

#endif // QUIN_GPU_SHADER_SHADER_TYPES_HPP
