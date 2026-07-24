#include "gpu/resource_translator.hpp"

namespace quin::gpu {

std::string ResourceTranslator::format_to_string(GnmSurfaceFormat format) {
    switch (format) {
        case GnmSurfaceFormat::R8G8B8A8_UNORM:     return "VK_FORMAT_R8G8B8A8_UNORM";
        case GnmSurfaceFormat::B8G8R8A8_UNORM:     return "VK_FORMAT_B8G8R8A8_UNORM";
        case GnmSurfaceFormat::R32_SFLOAT:         return "VK_FORMAT_R32_SFLOAT";
        case GnmSurfaceFormat::R16G16B16A16_SFLOAT: return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case GnmSurfaceFormat::D32_SFLOAT:         return "VK_FORMAT_D32_SFLOAT";
        default:                                   return "VK_FORMAT_UNDEFINED";
    }
}

std::string ResourceTranslator::topology_to_string(GnmPrimitiveType topology) {
    switch (topology) {
        case GnmPrimitiveType::PointList:     return "VK_PRIMITIVE_TOPOLOGY_POINT_LIST";
        case GnmPrimitiveType::LineList:      return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST";
        case GnmPrimitiveType::LineStrip:     return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP";
        case GnmPrimitiveType::TriangleList:  return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
        case GnmPrimitiveType::TriangleStrip: return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP";
        default:                              return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
    }
}

uint32_t ResourceTranslator::map_gnm_format_to_vk_format(GnmSurfaceFormat format) {
    // Vulkan numeric enum values matching VkFormat definitions
    switch (format) {
        case GnmSurfaceFormat::R8G8B8A8_UNORM:     return 37; // VK_FORMAT_R8G8B8A8_UNORM
        case GnmSurfaceFormat::B8G8R8A8_UNORM:     return 44; // VK_FORMAT_B8G8R8A8_UNORM
        case GnmSurfaceFormat::R32_SFLOAT:         return 100; // VK_FORMAT_R32_SFLOAT
        case GnmSurfaceFormat::R16G16B16A16_SFLOAT: return 97; // VK_FORMAT_R16G16B16A16_SFLOAT
        case GnmSurfaceFormat::D32_SFLOAT:         return 126; // VK_FORMAT_D32_SFLOAT
        default:                                   return 0;
    }
}

uint32_t ResourceTranslator::map_gnm_topology_to_vk_topology(GnmPrimitiveType topology) {
    switch (topology) {
        case GnmPrimitiveType::PointList:     return 0; // VK_PRIMITIVE_TOPOLOGY_POINT_LIST
        case GnmPrimitiveType::LineList:      return 1; // VK_PRIMITIVE_TOPOLOGY_LINE_LIST
        case GnmPrimitiveType::LineStrip:     return 2; // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP
        case GnmPrimitiveType::TriangleList:  return 3; // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        case GnmPrimitiveType::TriangleStrip: return 4; // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
        default:                              return 3;
    }
}

PsoKey ResourceTranslator::build_pso_key(const GnmContextState& context, GnmPrimitiveType topology) {
    return PsoKey{
        context.render_target_format,
        context.depth_target_format,
        topology,
        context.depth_test_enable,
        context.blend_enable
    };
}

} // namespace quin::gpu
