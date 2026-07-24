#ifndef QUIN_GPU_RESOURCE_TRANSLATOR_HPP
#define QUIN_GPU_RESOURCE_TRANSLATOR_HPP

#include "gpu/gnm_types.hpp"
#include <string>
#include <cstdint>

namespace quin::gpu {

struct PsoKey {
    GnmSurfaceFormat rt_format;
    GnmSurfaceFormat depth_format;
    GnmPrimitiveType primitive_type;
    bool depth_test;
    bool blend_enable;

    bool operator==(const PsoKey& other) const {
        return rt_format == other.rt_format &&
               depth_format == other.depth_format &&
               primitive_type == other.primitive_type &&
               depth_test == other.depth_test &&
               blend_enable == other.blend_enable;
    }
};

struct PsoKeyHash {
    size_t operator()(const PsoKey& key) const {
        size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t>(key.rt_format));
        size_t h2 = std::hash<uint32_t>{}(static_cast<uint32_t>(key.primitive_type));
        return h1 ^ (h2 << 1);
    }
};

class ResourceTranslator {
public:
    static std::string format_to_string(GnmSurfaceFormat format);
    static std::string topology_to_string(GnmPrimitiveType topology);

    static uint32_t map_gnm_format_to_vk_format(GnmSurfaceFormat format);
    static uint32_t map_gnm_topology_to_vk_topology(GnmPrimitiveType topology);

    static PsoKey build_pso_key(const GnmContextState& context, GnmPrimitiveType topology);
};

} // namespace quin::gpu

#endif // QUIN_GPU_RESOURCE_TRANSLATOR_HPP
