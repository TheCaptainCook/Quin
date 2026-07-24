#include <catch2/catch_test_macros.hpp>
#include "gpu/shader/shader_types.hpp"
#include "gpu/shader/shader_recompiler.hpp"
#include "gpu/shader/shader_cache.hpp"
#include <vector>

TEST_CASE("RDNA2 Shader Hash & Recompilation Engine", "[gpu][shader]") {
    quin::gpu::shader::ShaderRecompiler recompiler;

    // 1. Hash computation
    uint32_t sample_rdna2_code[] = { 0x7E000200, 0x7E020201, 0xBF810000 };
    quin::gpu::shader::ShaderHash hash1 = quin::gpu::shader::ShaderRecompiler::compute_hash(
        sample_rdna2_code, sizeof(sample_rdna2_code)
    );
    REQUIRE(hash1 != 0);

    // 2. Vertex Shader Recompilation
    auto vs_res = recompiler.recompile(
        sample_rdna2_code, sizeof(sample_rdna2_code), quin::gpu::shader::ShaderType::Vertex
    );
    REQUIRE(vs_res.success == true);
    REQUIRE(vs_res.shader.stage == quin::gpu::shader::ShaderType::Vertex);
    REQUIRE(vs_res.shader.spirv_code.size() >= 5);
    REQUIRE(vs_res.shader.spirv_code[0] == quin::gpu::shader::SPIRV_MAGIC_NUMBER);

    // 3. Pixel Shader Recompilation
    auto ps_res = recompiler.recompile(
        sample_rdna2_code, sizeof(sample_rdna2_code), quin::gpu::shader::ShaderType::Pixel
    );
    REQUIRE(ps_res.success == true);
    REQUIRE(ps_res.shader.stage == quin::gpu::shader::ShaderType::Pixel);
}

TEST_CASE("Persistent Binary Shader Cache", "[gpu][shader]") {
    quin::gpu::shader::ShaderCache cache;

    quin::gpu::shader::CompiledShader shader{};
    shader.hash = 0x123456789ABCDEF0ULL;
    shader.stage = quin::gpu::shader::ShaderType::Vertex;
    shader.spirv_code = { quin::gpu::shader::SPIRV_MAGIC_NUMBER, 0x00010500 };
    shader.is_valid = true;

    // 1. Initial lookup miss
    REQUIRE(cache.contains(shader.hash) == false);
    REQUIRE(cache.get(shader.hash) == nullptr);
    REQUIRE(cache.get_cache_misses() == 1);

    // 2. Insert into cache
    cache.put(shader);
    REQUIRE(cache.contains(shader.hash) == true);
    REQUIRE(cache.get_cached_shader_count() == 1);

    // 3. Cache lookup hit
    const auto* retrieved = cache.get(shader.hash);
    REQUIRE(retrieved != nullptr);
    REQUIRE(retrieved->hash == shader.hash);
    REQUIRE(cache.get_cache_hits() == 1);
}
