#include "Vulray/VulrayV2.h"

namespace vr
{
    Device::Device(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback, vk::DebugUtilsMessageSeverityFlagsEXT severity)
    {
        bool enableDebug = debugCallback != nullptr;

        std::vector<const char*> instanceExtensions = {};
        std::vector<const char*> instanceLayers = {};

        instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        instanceExtensions.push_back(VULRAY_VULKAN_SURFACE_EXTENSION_NAME);

        if (enableDebug)
        {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

        auto appInfo = vk::ApplicationInfo()
                           .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                           .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
                           .setApiVersion(VK_API_VERSION_1_3);

        auto instanceInfo = vk::InstanceCreateInfo()
                                .setPApplicationInfo(&appInfo)
                                .setEnabledExtensionCount(static_cast<uint32_t>(instanceExtensions.size()))
                                .setPpEnabledExtensionNames(instanceExtensions.data())
                                .setEnabledLayerCount(static_cast<uint32_t>(instanceLayers.size()))
                                .setPpEnabledLayerNames(instanceLayers.data());

        mInstance = vk::createInstance(instanceInfo);

        mDynLoader.init(mInstance, vkGetInstanceProcAddr);

        if (enableDebug)
        {
            auto debugInfo = vk::DebugUtilsMessengerCreateInfoEXT()
                                 .setMessageSeverity(severity)
                                 .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                 vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
                                 .setPfnUserCallback(debugCallback);

            mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugInfo, nullptr, mDynLoader);
        }

        std::vector<const char*> deviceExtensions = {};

        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        deviceExtensions.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);

        std::vector<vk::PhysicalDevice> physicalDevices = mInstance.enumeratePhysicalDevices();

        for (auto& physicalDevice : physicalDevices)
        {
            if (CheckPhysicalDeviceProperties(physicalDevice))
            {
                mPhysicalDevice = physicalDevice;
                break;
            }
        }
        if (!mPhysicalDevice)
        {
            throw std::runtime_error("No suitable physical device found");
        }
    }

    Device::~Device()
    {
        mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger, nullptr, mDynLoader);
        mInstance.destroy();
    }

    bool Device::CheckPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice)
    {
        auto dev13Features = vk::PhysicalDeviceVulkan13Features();
        auto dev12Features = vk::PhysicalDeviceVulkan12Features().setPNext(&dev13Features);
        auto dev11Features = vk::PhysicalDeviceVulkan11Features().setPNext(&dev12Features);

        auto features2 = vk::PhysicalDeviceFeatures2().setPNext(&dev11Features);

        physicalDevice.getFeatures2(&features2);

        return true;
    }
} // namespace vr
