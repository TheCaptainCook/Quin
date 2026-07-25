#include "gpu/vulkan_backend.hpp"
#include "core/logging.hpp"
#include <sstream>

namespace quin::gpu {

VulkanBackend::VulkanBackend() = default;

VulkanBackend::~VulkanBackend() {
    shutdown();
}

bool VulkanBackend::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_device_info.initialized) return true;

#ifdef QUIN_HAS_VULKAN
    // =====================================================================
    // Real Vulkan Initialization — detect actual GPU hardware
    // =====================================================================

    // 1. Create VkInstance
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Quin PS5 Emulator";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Quin Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkResult result = vkCreateInstance(&create_info, nullptr, &m_vk_instance);
    if (result != VK_SUCCESS) {
        QUIN_LOG_WARN("VulkanBackend: vkCreateInstance failed (VkResult={}). Falling back to simulated mode.", static_cast<int>(result));
        goto fallback;
    }

    // 2. Enumerate physical devices
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_vk_instance, &device_count, nullptr);
        if (device_count == 0) {
            QUIN_LOG_WARN("VulkanBackend: No Vulkan-compatible GPU found. Falling back to simulated mode.");
            vkDestroyInstance(m_vk_instance, nullptr);
            m_vk_instance = VK_NULL_HANDLE;
            goto fallback;
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_vk_instance, &device_count, devices.data());
        m_vk_physical_device = devices[0]; // Select first device

        // 3. Get device properties
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(m_vk_physical_device, &props);

        m_device_info.device_name = props.deviceName;
        m_device_info.api_version = props.apiVersion;

        uint32_t drv_major = VK_VERSION_MAJOR(props.driverVersion);
        uint32_t drv_minor = VK_VERSION_MINOR(props.driverVersion);
        uint32_t drv_patch = VK_VERSION_PATCH(props.driverVersion);
        std::ostringstream drv_ss;
        drv_ss << drv_major << "." << drv_minor << "." << drv_patch;
        m_device_info.driver_version = drv_ss.str();

        // 4. Get memory properties for VRAM size
        VkPhysicalDeviceMemoryProperties mem_props{};
        vkGetPhysicalDeviceMemoryProperties(m_vk_physical_device, &mem_props);
        uint64_t total_vram = 0;
        for (uint32_t i = 0; i < mem_props.memoryHeapCount; ++i) {
            if (mem_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                total_vram += mem_props.memoryHeaps[i].size;
            }
        }
        m_device_info.vram_bytes = total_vram;

        // 5. Create logical device (minimal — single graphics queue)
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_vk_physical_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_vk_physical_device, &queue_family_count, queue_families.data());

        uint32_t graphics_family = UINT32_MAX;
        for (uint32_t i = 0; i < queue_family_count; ++i) {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_family = i;
                break;
            }
        }

        if (graphics_family != UINT32_MAX) {
            float queue_priority = 1.0f;
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = graphics_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;

            VkDeviceCreateInfo device_create_info{};
            device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_create_info.queueCreateInfoCount = 1;
            device_create_info.pQueueCreateInfos = &queue_create_info;

            result = vkCreateDevice(m_vk_physical_device, &device_create_info, nullptr, &m_vk_device);
            if (result != VK_SUCCESS) {
                QUIN_LOG_WARN("VulkanBackend: vkCreateDevice failed (VkResult={})", static_cast<int>(result));
            }
        }

        m_device_info.initialized = true;
        m_device_info.real_vulkan = true;

        QUIN_LOG_INFO("VulkanBackend: Real Vulkan initialized — GPU: '{}' | Driver: {} | VRAM: {} MB | API: {}.{}.{}",
                      m_device_info.device_name, m_device_info.driver_version,
                      m_device_info.vram_bytes / (1024 * 1024),
                      VK_VERSION_MAJOR(m_device_info.api_version),
                      VK_VERSION_MINOR(m_device_info.api_version),
                      VK_VERSION_PATCH(m_device_info.api_version));
        return true;
    }

fallback:
#endif
    // =====================================================================
    // Simulated mode — no Vulkan SDK or no compatible GPU
    // =====================================================================
    m_device_info.device_name = "Simulated GPU (No Vulkan SDK)";
    m_device_info.driver_version = "N/A";
    m_device_info.api_version = 0;
    m_device_info.vram_bytes = 0;
    m_device_info.initialized = true;
    m_device_info.real_vulkan = false;

    QUIN_LOG_WARN("VulkanBackend: Running in SIMULATED mode — Vulkan SDK not linked or no GPU found.");
    QUIN_LOG_WARN("VulkanBackend: To enable real GPU detection, build with -DQUIN_HAS_VULKAN and link Vulkan::Vulkan.");
    return true;
}

void VulkanBackend::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_device_info.initialized) return;

#ifdef QUIN_HAS_VULKAN
    if (m_vk_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_vk_device, nullptr);
        m_vk_device = VK_NULL_HANDLE;
    }
    if (m_vk_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_vk_instance, nullptr);
        m_vk_instance = VK_NULL_HANDLE;
    }
#endif

    m_pso_cache.clear();
    m_device_info = VulkanDeviceInfo{};
    QUIN_LOG_INFO("VulkanBackend Shutdown cleanly.");
}

uint64_t VulkanBackend::get_or_create_pipeline(const PsoKey& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_pso_cache.find(key);
    if (it != m_pso_cache.end()) {
        it->second.cache_hit_count++;
        m_total_pso_cache_hits++;
        return it->second.pipeline_handle;
    }

    // Create new pipeline entry (tracked for future real Vulkan PSO compilation)
    uint64_t pipeline_id = m_next_pipeline_id++;
    DummyPipeline pipeline{key, pipeline_id, 0};
    m_pso_cache[key] = pipeline;

    QUIN_LOG_INFO("VulkanBackend: Created PSO #{} — RT: {} | Depth: {} | Prim: {} | DepthTest: {} | Blend: {}",
                  pipeline_id,
                  ResourceTranslator::format_to_string(key.rt_format),
                  ResourceTranslator::format_to_string(key.depth_format),
                  ResourceTranslator::topology_to_string(key.primitive_type),
                  key.depth_test ? "ON" : "OFF",
                  key.blend_enable ? "ON" : "OFF");

    return pipeline_id;
}

void VulkanBackend::submit_command_buffer(const ParseResult& parse_result) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& draw_cmd : parse_result.draw_commands) {
        PsoKey key = ResourceTranslator::build_pso_key(parse_result.context_state, draw_cmd.primitive_type);
        auto it = m_pso_cache.find(key);
        if (it == m_pso_cache.end()) {
            uint64_t pid = m_next_pipeline_id++;
            m_pso_cache[key] = DummyPipeline{key, pid, 0};
        }
        m_total_draw_calls_rendered++;
    }
}

size_t VulkanBackend::get_cached_pipelines_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pso_cache.size();
}

} // namespace quin::gpu
