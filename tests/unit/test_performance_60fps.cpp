#include <catch2/catch_test_macros.hpp>
#include "gpu/pso_disk_cache.hpp"
#include "gpu/frame_pacing.hpp"
#include "gpu/shader/async_shader_compiler.hpp"
#include <thread>
#include <chrono>

TEST_CASE("PSO Disk Cache Serialization & Persistence", "[gpu][performance]") {
    quin::gpu::PsoDiskCache disk_cache("./test_cache/");

    quin::gpu::PsoKey key1{};
    key1.rt_format = quin::gpu::GnmSurfaceFormat::R8G8B8A8_UNORM;
    key1.depth_format = quin::gpu::GnmSurfaceFormat::R32_SFLOAT;
    key1.primitive_type = quin::gpu::GnmPrimitiveType::TriangleList;
    key1.depth_test = true;
    key1.blend_enable = false;

    disk_cache.put_record(key1, 4004);
    REQUIRE(disk_cache.get_records_count() == 1);

    quin::gpu::DiskPsoRecord out_rec{};
    REQUIRE(disk_cache.get_record(key1, out_rec) == true);
    REQUIRE(out_rec.pipeline_id == 4004);

    REQUIRE(disk_cache.save_to_disk() == true);

    quin::gpu::PsoDiskCache disk_cache_reloaded("./test_cache/");
    REQUIRE(disk_cache_reloaded.load_from_disk() == true);
    REQUIRE(disk_cache_reloaded.get_records_count() == 1);

    quin::gpu::DiskPsoRecord reloaded_rec{};
    REQUIRE(disk_cache_reloaded.get_record(key1, reloaded_rec) == true);
    REQUIRE(reloaded_rec.pipeline_id == 4004);
}

TEST_CASE("Async Shader Compiler Worker Pool", "[gpu][shader][performance]") {
    quin::gpu::shader::ShaderRecompiler recompiler;
    quin::gpu::shader::ShaderCache cache;

    quin::gpu::shader::AsyncShaderCompiler async_compiler(recompiler, cache, 2);
    async_compiler.start();
    REQUIRE(async_compiler.get_worker_threads_count() == 2);

    uint32_t dummy_bytecode[] = { 0x7E000200, 0x7E020201, 0xBF810000 };
    async_compiler.queue_job(dummy_bytecode, sizeof(dummy_bytecode), quin::gpu::shader::ShaderType::Vertex);

    // Wait briefly for worker thread execution
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    async_compiler.stop();
    REQUIRE(cache.get_cached_shader_count() >= 1);
}

TEST_CASE("Frame Pacing Regulator & Dynamic Resolution Scaling", "[gpu][performance]") {
    quin::gpu::FramePacingRegulator regulator;

    // 1. Initial State
    REQUIRE(regulator.get_mode() == quin::gpu::FramePacingMode::Locked60);
    REQUIRE(regulator.get_resolution_scale() == 1.0f);

    // 2. Mode Change
    regulator.set_mode(quin::gpu::FramePacingMode::Locked30);
    REQUIRE(regulator.get_mode() == quin::gpu::FramePacingMode::Locked30);

    regulator.set_mode(quin::gpu::FramePacingMode::Unlocked);
    REQUIRE(regulator.get_mode() == quin::gpu::FramePacingMode::Unlocked);

    // 3. Resolution Scaling Bounds
    regulator.set_resolution_scale(0.75f);
    REQUIRE(regulator.get_resolution_scale() == 0.75f);

    regulator.set_resolution_scale(1.5f); // Should clamp to 1.0f
    REQUIRE(regulator.get_resolution_scale() == 1.0f);

    regulator.set_resolution_scale(0.2f); // Should clamp to 0.5f
    REQUIRE(regulator.get_resolution_scale() == 0.5f);

    // 4. Frame timing calculation
    regulator.begin_frame();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    regulator.end_frame();

    REQUIRE(regulator.get_last_frame_time_ms() > 0.0);
}
