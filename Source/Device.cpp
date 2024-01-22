#include "Vulray/VulrayV2.h"

// static default debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                           void* pUserData)
{
    std::string severity;
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "VERBOSE"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "INFO"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "WARNING"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "ERROR"; break;
    default: severity = "UNKNOWN"; break;
    }

    std::string type;
    switch (messageType)
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: type = "GENERAL"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: type = "VALIDATION"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: type = "PERFORMANCE"; break;
    default: type = "UNKNOWN"; break;
    }

    std::cout << "[" << severity << " " << type << "] " << pCallbackData->pMessage << "\n";

    return VK_FALSE;
}

namespace vr
{
    VulkanContext CreateVulkanContext(DeviceRequirements& req)
    {
        VulkanContext outCtx = {};

        auto appInfo = vk::ApplicationInfo()
                           .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                           .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
                           .setApiVersion(VK_API_VERSION_1_3);

        // Check for validation layers and debug messenger
        if (req.EnableValidation)
            req.InstanceLayers.push_back("VK_LAYER_KHRONOS_validation");

        if (req.EnableDebugMessenger)
        {
            req.InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        req.InstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        // Set the extension for the surface creation function.
#ifdef _WIN32

        req.InstanceExtensions.push_back("VK_KHR_win32_surface");

#elif __linux__

        req.InstanceExtensions.push_back("VK_KHR_xcb_surface");

#endif // _WIN32

        // Check if the instance extensions are supported
        {
            auto supportedExtensions = vk::enumerateInstanceExtensionProperties();

            for (auto& ext : req.InstanceExtensions)
            {
                bool found = false;
                for (auto& supportedExt : supportedExtensions)
                {
                    if (strcmp(ext, supportedExt.extensionName) == 0)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    throw std::runtime_error("Required instance extension not supported");
                }
            }
        }

        // Now create the instance since all the checks have passed
        auto instanceCreateInfo = vk::InstanceCreateInfo()
                                      .setPApplicationInfo(&appInfo)
                                      .setEnabledExtensionCount(static_cast<uint32_t>(req.InstanceExtensions.size()))
                                      .setPpEnabledExtensionNames(req.InstanceExtensions.data())
                                      .setEnabledLayerCount(static_cast<uint32_t>(req.InstanceLayers.size()))
                                      .setPpEnabledLayerNames(req.InstanceLayers.data());

        outCtx.Instance = vk::createInstance(instanceCreateInfo);

        // Load the dynamic loader
        outCtx.DynLoader.init(outCtx.Instance, vkGetInstanceProcAddr);

        // Load the debug messenger
        if (req.EnableDebugMessenger)
        {
            auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT()
                                  .setMessageSeverity(req.MessageSeverity)
                                  .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                  vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                  vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
                                  .setPfnUserCallback(req.DebugCallback ? req.DebugCallback : DefaultDebugCallback)
                                  .setPUserData(req.DebugCallbackUserData);

            outCtx.DebugMessenger = outCtx.Instance.createDebugUtilsMessengerEXT(createInfo, nullptr, outCtx.DynLoader);
        }

        // Add the raytracing extensions and descriptor buffer extension
        req.DeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        req.DeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        req.DeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        req.DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);

        // Add any features that are needed
        req.PhysicalDeviceFeatures12.bufferDeviceAddress = true;

        auto rtPipelineFeatures =
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR().setRayTracingPipeline(true).setPNext(req.FeatureChain);

        auto accelStructureFeatures =
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR().setAccelerationStructure(true).setPNext(
                &rtPipelineFeatures);

        auto descriptorBufferFeatures =
            vk::PhysicalDeviceDescriptorBufferFeaturesEXT().setDescriptorBuffer(true).setPNext(&accelStructureFeatures);

        void* featureChain = &descriptorBufferFeatures;

        // Check if the physical devices support the needed extensions
        {
            // Get the physical devices
            auto physicalDevices = outCtx.Instance.enumeratePhysicalDevices();

            for (auto& physicalDevice : physicalDevices)
            {
                // Check the extensions
                auto supportedExtensions = physicalDevice.enumerateDeviceExtensionProperties();

                bool hasExtensions = true;
                for (auto& ext : req.DeviceExtensions)
                {
                    // Sequential search through the supported extensions
                    bool found = false;
                    for (auto& supportedExt : supportedExtensions)
                    {
                        if (strcmp(ext, supportedExt.extensionName) == 0)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        hasExtensions = false;
                        break;
                    }
                }

                if (!hasExtensions)
                    continue;
                else
                {
                    outCtx.PhysicalDevice = physicalDevice;
                    break;
                }
            }

            if (!outCtx.PhysicalDevice)
            {
                throw std::runtime_error("No physical device found that supports the needed extensions");
            }
        }

        VULRAY_ASSERT(!(req.DedicatedCompute && req.SeparateComputeTransferQueue &&
                        "Cannot have DedicatedCompute and SeparateComputeTransferQueue at the same time"));
        VULRAY_ASSERT(!(req.DedicatedTransfer && req.SeparateComputeTransferQueue &&
                        "Cannot have DedicatedTransfer and SeparateComputeTransferQueue at the same time"));

        // Create the logical device
        {
            // Get the queue families
            auto queueFamilies = outCtx.PhysicalDevice.getQueueFamilyProperties();

            // Find the queue families, if dedicated queues are needed, find the dedicated queues
            uint32_t graphicsIndex = ~0U;
            uint32_t computeIndex = ~0U;
            uint32_t transferIndex = ~0U;

            for (uint32_t i = 0; i < queueFamilies.size(); ++i)
            {
                auto& queueFamily = queueFamilies[i];

                if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                {
                    graphicsIndex = i;
                }

                if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
                {
                    if (req.DedicatedCompute)
                        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ||
                            queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
                            continue;

                    if (req.SeparateComputeTransferQueue)
                        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                            continue;

                    computeIndex = i;
                }

                if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
                {
                    if (req.SeparateComputeTransferQueue)
                        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                            continue;

                    if (req.DedicatedTransfer)
                        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ||
                            queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
                            continue;

                    transferIndex = i;
                }
            }

            if (graphicsIndex == ~0U)
                throw std::runtime_error("No graphics queue found");
            if (req.DedicatedCompute && computeIndex == ~0U)
                throw std::runtime_error("No dedicated compute queue found");
            if (req.DedicatedTransfer && transferIndex == ~0U)
                throw std::runtime_error("No dedicated transfer queue found");

            // Create the queue create infos
            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = {};

            float priority = 1.0f;

            // Graphics queue
            {
                auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                                           .setQueueFamilyIndex(graphicsIndex)
                                           .setQueueCount(1)
                                           .setPQueuePriorities(&priority);

                queueCreateInfos.push_back(queueCreateInfo);
            }

            // Compute queue
            if (computeIndex != graphicsIndex)
            {
                auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                                           .setQueueFamilyIndex(computeIndex)
                                           .setQueueCount(1)
                                           .setPQueuePriorities(&priority);
                queueCreateInfos.push_back(queueCreateInfo);
            }

            // Transfer queue
            if (transferIndex != graphicsIndex && transferIndex != computeIndex)
            {
                auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                                           .setQueueFamilyIndex(transferIndex)
                                           .setQueueCount(1)
                                           .setPQueuePriorities(&priority);
                queueCreateInfos.push_back(queueCreateInfo);
            }

            float queuePriority = 1.0f;

            // Prepare the features chain
            auto features = vk::PhysicalDeviceFeatures2()
                                .setFeatures(req.PhysicalDeviceFeatures10)
                                .setPNext(&req.PhysicalDeviceFeatures11);

            req.PhysicalDeviceFeatures11.setPNext(&req.PhysicalDeviceFeatures12);
            req.PhysicalDeviceFeatures12.setPNext(&req.PhysicalDeviceFeatures13);
            req.PhysicalDeviceFeatures13.setPNext(featureChain);

            auto logicalDeviceCreateInfo =
                vk::DeviceCreateInfo()
                    .setPQueueCreateInfos(queueCreateInfos.data())
                    .setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
                    .setPNext(&features)
                    .setEnabledExtensionCount(static_cast<uint32_t>(req.DeviceExtensions.size()))
                    .setPpEnabledExtensionNames(req.DeviceExtensions.data());

            outCtx.Device = outCtx.PhysicalDevice.createDevice(logicalDeviceCreateInfo);

            // Get the queues
            outCtx.GraphicsQueue = outCtx.Device.getQueue(graphicsIndex, 0);
            outCtx.ComputeQueue = outCtx.Device.getQueue(computeIndex, 0);
            outCtx.TransferQueue = outCtx.Device.getQueue(transferIndex, 0);
        }
    }

    void DestroyVulkanContext(VulkanContext& context)
    {
        if (context.Device)
        {
            context.Device.destroy();
            context.Device = nullptr;
        }

        if (context.DebugMessenger)
        {
            context.Instance.destroyDebugUtilsMessengerEXT(context.DebugMessenger, nullptr, context.DynLoader);
            context.DebugMessenger = nullptr;
        }

        if (context.Instance)
        {
            context.Instance.destroy();
            context.Instance = nullptr;
        }
    }

    Device::Device(const VulkanContext& context) : mCtx(context)
    {
    }

    Device::~Device()
    {
    }

} // namespace vr
