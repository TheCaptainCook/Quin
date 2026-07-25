#ifndef QUIN_GPU_GNM_TYPES_HPP
#define QUIN_GPU_GNM_TYPES_HPP

#include <cstdint>
#include <vector>

namespace quin::gpu {

// PM4 Packet Constants
constexpr uint32_t PM4_TYPE3_HEADER = 0xC0000000;

enum GnmOpcode : uint8_t {
    IT_NOP                = 0x10,
    IT_SET_BASE           = 0x11,
    IT_INDEX_TYPE         = 0x2A,
    IT_NUM_INSTANCES      = 0x2D,
    IT_DRAW_INDEX_AUTO    = 0x2D,
    IT_DRAW_INDEX_2       = 0x36,
    IT_EVENT_WRITE        = 0x46,
    IT_WAIT_REG_MEM       = 0x3C,
    IT_SET_CONTEXT_REG    = 0x69
};

enum class GnmSurfaceFormat : uint32_t {
    Invalid = 0,
    R8G8B8A8_UNORM,
    B8G8R8A8_UNORM,
    R32_SFLOAT,
    R16G16B16A16_SFLOAT,
    D32_SFLOAT
};

enum class GnmPrimitiveType : uint32_t {
    PointList = 0,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip
};

struct GnmPm4Header {
    uint8_t count;
    uint8_t opcode;
    uint8_t type;
};

struct GnmDrawCommand {
    GnmPrimitiveType primitive_type{GnmPrimitiveType::TriangleList};
    uint32_t index_count{0};
    uint32_t instance_count{1};
    uint32_t first_index{0};
    uint64_t vertex_buffer_vaddr{0};
    uint64_t index_buffer_vaddr{0};
};

struct GnmContextState {
    GnmSurfaceFormat render_target_format{GnmSurfaceFormat::R8G8B8A8_UNORM};
    GnmSurfaceFormat depth_target_format{GnmSurfaceFormat::D32_SFLOAT};
    GnmSurfaceFormat depth_format{GnmSurfaceFormat::D32_SFLOAT};
    uint32_t viewport_width{1920};
    uint32_t viewport_height{1080};
    bool depth_test_enable{true};
    bool depth_write_enable{true};
    bool blend_enable{false};
    bool index_size_16bit{true};
};

} // namespace quin::gpu

#endif // QUIN_GPU_GNM_TYPES_HPP
