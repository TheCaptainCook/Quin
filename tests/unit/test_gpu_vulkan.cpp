#include <catch2/catch_test_macros.hpp>
#include "gpu/gnm_types.hpp"
#include "gpu/gnm_parser.hpp"
#include "gpu/resource_translator.hpp"
#include "gpu/vulkan_backend.hpp"
#include "memory/address_space.hpp"
#include <vector>

TEST_CASE("GNM PM4 Command Buffer Parsing", "[gpu][gnm]") {
    quin::memory::GuestAddressSpace memory;
    quin::gpu::GnmCmdParser parser(memory);

    // Map 4KB guest memory for PM4 command ring
    uint64_t ring_vaddr = memory.mmap(0x0000000000500000ULL, 4096, quin::memory::PagePermission::ReadWrite);

    // Construct synthetic PM4 IT_DRAW_INDEX_AUTO packet
    // Header word: Count=1 (2 DWORD payload), Opcode=0x2D (IT_DRAW_INDEX_AUTO), Type=3 (0xC0000000)
    uint32_t count = 1;
    uint32_t opcode = quin::gpu::IT_DRAW_INDEX_AUTO;
    uint32_t type = 3;
    uint32_t header = quin::gpu::PM4_TYPE3_HEADER | (count << 16) | (opcode << 8);

    std::vector<uint32_t> pm4_words = {
        header,
        36, // Index count
        3   // Primitive type = TriangleList
    };

    REQUIRE(memory.write_bytes(ring_vaddr, pm4_words.data(), pm4_words.size() * sizeof(uint32_t)) == true);

    auto result = parser.parse_command_buffer(ring_vaddr, pm4_words.size());
    REQUIRE(result.success == true);
    REQUIRE(result.draw_commands.size() == 1);
    REQUIRE(result.draw_commands[0].index_count == 36);
    REQUIRE(result.draw_commands[0].primitive_type == quin::gpu::GnmPrimitiveType::TriangleList);
}

TEST_CASE("GNM to Vulkan Resource & Topology Translator", "[gpu][vulkan]") {
    // 1. Format string mapping
    std::string fmt_str = quin::gpu::ResourceTranslator::format_to_string(quin::gpu::GnmSurfaceFormat::R8G8B8A8_UNORM);
    REQUIRE(fmt_str == "VK_FORMAT_R8G8B8A8_UNORM");

    // 2. Topology string mapping
    std::string top_str = quin::gpu::ResourceTranslator::topology_to_string(quin::gpu::GnmPrimitiveType::TriangleStrip);
    REQUIRE(top_str == "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP");

    // 3. VkFormat mapping
    uint32_t vk_fmt = quin::gpu::ResourceTranslator::map_gnm_format_to_vk_format(quin::gpu::GnmSurfaceFormat::R8G8B8A8_UNORM);
    REQUIRE(vk_fmt == 37);

    // 4. PSO Key Hash
    quin::gpu::GnmContextState ctx{};
    ctx.render_target_format = quin::gpu::GnmSurfaceFormat::R8G8B8A8_UNORM;
    ctx.depth_target_format = quin::gpu::GnmSurfaceFormat::D32_SFLOAT;

    auto key1 = quin::gpu::ResourceTranslator::build_pso_key(ctx, quin::gpu::GnmPrimitiveType::TriangleList);
    auto key2 = quin::gpu::ResourceTranslator::build_pso_key(ctx, quin::gpu::GnmPrimitiveType::TriangleList);

    REQUIRE(key1 == key2);
}

TEST_CASE("Vulkan Graphics Backend & PSO Pipeline Cache", "[gpu][vulkan]") {
    quin::gpu::VulkanBackend backend;
    REQUIRE(backend.initialize() == true);

    const auto& dev = backend.get_device_info();
    REQUIRE(dev.initialized == true);
    REQUIRE(dev.vram_bytes > 0);

    // Get or compile PSO pipeline
    quin::gpu::PsoKey key{};
    key.rt_format = quin::gpu::GnmSurfaceFormat::R8G8B8A8_UNORM;
    key.primitive_type = quin::gpu::GnmPrimitiveType::TriangleList;

    uint64_t pso1 = backend.get_or_create_pipeline(key);
    REQUIRE(pso1 > 0);

    // Second request hits cache
    uint64_t pso2 = backend.get_or_create_pipeline(key);
    REQUIRE(pso1 == pso2);
    REQUIRE(backend.get_total_pso_cache_hits() == 1);
}
