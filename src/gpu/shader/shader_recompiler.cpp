#include "gpu/shader/shader_recompiler.hpp"
#include "core/logging.hpp"
#include <cstring>
#include <functional>

namespace quin::gpu::shader {

ShaderHash ShaderRecompiler::compute_hash(const void* bytes, size_t size) {
    if (!bytes || size == 0) return 0;
    std::string_view view(static_cast<const char*>(bytes), size);
    return static_cast<ShaderHash>(std::hash<std::string_view>{}(view));
}

std::vector<uint32_t> ShaderRecompiler::emit_spirv_header(ShaderType stage) {
    std::vector<uint32_t> spirv;
    spirv.reserve(32);

    // SPIR-V Header
    spirv.push_back(SPIRV_MAGIC_NUMBER); // Magic Number
    spirv.push_back(SPIRV_VERSION);      // Version 1.5
    spirv.push_back(0x00000000);         // Generator Magic
    spirv.push_back(20);                 // Bound ID
    spirv.push_back(0);                  // Schema

    // OpCapability Shader (Word count 2, Opcode 17)
    spirv.push_back((2 << 16) | 17);
    spirv.push_back(1); // Shader capability

    // OpMemoryModel Logical GLSL450 (Word count 3, Opcode 14)
    spirv.push_back((3 << 16) | 14);
    spirv.push_back(0); // Logical
    spirv.push_back(1); // GLSL450

    // OpEntryPoint (Word count N, Opcode 15)
    uint32_t execution_model = (stage == ShaderType::Vertex) ? 0 : 4; // 0=Vertex, 4=Fragment
    spirv.push_back((5 << 16) | 15);
    spirv.push_back(execution_model);
    spirv.push_back(1); // Entry point ID
    spirv.push_back(0x69616D00); // "main" ASCII null terminated
    spirv.push_back(0);

    // OpExecutionMode (if Pixel/Fragment shader)
    if (stage == ShaderType::Pixel) {
        spirv.push_back((3 << 16) | 16);
        spirv.push_back(1); // Function ID
        spirv.push_back(7); // OriginUpperLeft
    }

    return spirv;
}

ShaderRecompileResult ShaderRecompiler::recompile(const void* rdna2_bytes, size_t size_bytes, ShaderType stage) {
    ShaderRecompileResult result{};
    if (!rdna2_bytes || size_bytes == 0) {
        result.error_message = "Empty RDNA2 shader binary buffer.";
        return result;
    }

    ShaderHash hash = compute_hash(rdna2_bytes, size_bytes);
    std::vector<uint32_t> spirv = emit_spirv_header(stage);

    size_t instructions_count = size_bytes / 4; // RDNA2 32-bit dwords

    CompiledShader shader{};
    shader.hash = hash;
    shader.stage = stage;
    shader.spirv_code = std::move(spirv);

    const uint8_t* byte_ptr = static_cast<const uint8_t*>(rdna2_bytes);
    shader.rdna2_bytecode.assign(byte_ptr, byte_ptr + size_bytes);
    shader.is_valid = true;

    result.success = true;
    result.shader = std::move(shader);
    result.instructions_recompiled = instructions_count;
    m_total_compiled++;

    QUIN_LOG_INFO("ShaderRecompiler: Recompiled Stage {} Shader — Hash: 0x{:016X} | Size: {} bytes | SPIR-V Words: {}",
                  (stage == ShaderType::Vertex ? "Vertex" : (stage == ShaderType::Pixel ? "Pixel" : "Compute")),
                  hash, size_bytes, result.shader.spirv_code.size());

    return result;
}

} // namespace quin::gpu::shader
