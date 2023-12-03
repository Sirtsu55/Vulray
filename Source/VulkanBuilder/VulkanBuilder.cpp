#include "Vulray/VulkanBuilder/VulkanBuilder.h"

#include "VkBootstrap.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL VulrayVulkanDebugCback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                             void* pUserData);

namespace vr
{

    struct _BuilderVKBStructs
    {
        vkb::Instance Instance;
        vkb::PhysicalDevice PhysicalDevice;
        vkb::Device Device;
    };
    static std::vector<const char*> RayTracingExtensions = {
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,     VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,   VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // required by accel struct extension
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, // Required by Vulray if using Descriptors that vulray creates
    };

    std::vector<const char*> GetRequiredExtensionsForVulray()
    {
        return RayTracingExtensions;
    }

    VulkanBuilder::VulkanBuilder()
    {
        _StructData = std::make_shared<_BuilderVKBStructs>();
    }

    VulkanBuilder::~VulkanBuilder()
    {
    }

    InstanceWrapper VulkanBuilder::CreateInstance()
    {
        auto instBuilder = vkb::InstanceBuilder().require_api_version(1, 3, 0);

        for (auto v : ValidationFeatures)
            instBuilder.add_validation_feature_enable(static_cast<VkValidationFeatureEnableEXT>(v));
        for (auto& ext : InstanceExtensions) instBuilder.enable_extension(ext);
        for (auto& layer : InstanceLayers) instBuilder.enable_layer(layer);

        // required extensions by Vulray
        instBuilder.enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        if (EnableDebug)
        {
            instBuilder.request_validation_layers()
                .set_debug_callback_user_data_pointer(DebugCallbackUserData)
                .set_debug_callback(DebugCallback == nullptr ? VulrayVulkanDebugCback : DebugCallback)
                .set_debug_messenger_severity(
                    (VkDebugUtilsMessageSeverityFlagsEXT)(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose))
                .add_debug_messenger_type(
                    (VkDebugUtilsMessageTypeFlagsEXT)(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance));
        }

        auto instanceResult = instBuilder.build();
        if (!instanceResult.has_value())
        {
            VULRAY_FLOG_ERROR("Instance Build failed, Error: %s", instanceResult.error().message());
            throw std::runtime_error("No Instance Created");
            return {};
        }

        vkb::Instance& returnStructs = reinterpret_cast<_BuilderVKBStructs*>(_StructData.get())->Instance;
        returnStructs = instanceResult.value();

        return {returnStructs.instance, returnStructs.debug_messenger};
    }

    vk::PhysicalDevice VulkanBuilder::PickPhysicalDevice(vk::SurfaceKHR surface)
    {
        vkb::Instance& instance = reinterpret_cast<_BuilderVKBStructs*>(_StructData.get())->Instance;

        auto physSelector = vkb::PhysicalDeviceSelector(instance, surface)
                                .add_required_extensions(DeviceExtensions)
                                .add_required_extensions(RayTracingExtensions)
                                .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                                .set_surface(surface)
                                .require_present();

        // Enable needed features
        auto raytracingFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR().setRayTracingPipeline(true);

        auto rayqueryFeatures = vk::PhysicalDeviceRayQueryFeaturesKHR().setRayQuery(true);

        auto accelFeatures = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
                                 .setAccelerationStructure(true)
                                 .setDescriptorBindingAccelerationStructureUpdateAfterBind(true);

        auto descbufferFeatures = vk::PhysicalDeviceDescriptorBufferFeaturesEXT()
                                      .setDescriptorBuffer(true)
                                      .setDescriptorBufferImageLayoutIgnored(true);

        physSelector.add_required_extension_features(raytracingFeatures);
        physSelector.add_required_extension_features(rayqueryFeatures);
        physSelector.add_required_extension_features(accelFeatures);
        physSelector.add_required_extension_features(descbufferFeatures);

        PhysicalDeviceFeatures12.bufferDeviceAddress = true;
        PhysicalDeviceFeatures12.descriptorIndexing = true;
        PhysicalDeviceFeatures12.descriptorBindingVariableDescriptorCount = true;
        PhysicalDeviceFeatures12.descriptorBindingPartiallyBound = true;
        PhysicalDeviceFeatures12.runtimeDescriptorArray = true;

        PhysicalDeviceFeatures12.shaderSampledImageArrayNonUniformIndexing = true;
        PhysicalDeviceFeatures12.shaderStorageBufferArrayNonUniformIndexing = true;
        PhysicalDeviceFeatures12.shaderStorageImageArrayNonUniformIndexing = true;
        PhysicalDeviceFeatures12.shaderUniformBufferArrayNonUniformIndexing = true;
        PhysicalDeviceFeatures12.shaderUniformTexelBufferArrayNonUniformIndexing = true;
        PhysicalDeviceFeatures12.shaderStorageTexelBufferArrayNonUniformIndexing = true;

        physSelector.set_required_features(PhysicalDeviceFeatures10);
        physSelector.set_required_features_11(PhysicalDeviceFeatures11);
        physSelector.set_required_features_12(PhysicalDeviceFeatures12);
        physSelector.set_required_features_13(PhysicalDeviceFeatures13);

        physSelector.set_minimum_version(1, 3);
        auto physResult = physSelector.select();
        if (!physResult.has_value())
        {
            VULRAY_FLOG_ERROR("Physical Device Build failed, Error: %s", physResult.error().message());
            throw std::runtime_error("Physical Device Build failed");
            return {};
        }

        vkb::PhysicalDevice& returnStruct = reinterpret_cast<_BuilderVKBStructs*>(_StructData.get())->PhysicalDevice;
        returnStruct = physResult.value();

        return returnStruct.physical_device;
    }

    vk::Device VulkanBuilder::CreateDevice()
    {
        vkb::PhysicalDevice& physDev = reinterpret_cast<_BuilderVKBStructs*>(_StructData.get())->PhysicalDevice;

        auto devBuidler = vkb::DeviceBuilder(physDev);

        auto devResult = devBuidler.build();

        if (!devResult.has_value())
        {
            VULRAY_FLOG_ERROR("Logical Device Build failed, Error: %s", devResult.error().message());
            throw std::runtime_error("No Logical Devices Created");
            return {};
        }
        vkb::Device& dev = reinterpret_cast<_BuilderVKBStructs*>(_StructData.get())->Device;
        dev = devResult.value();

        return dev.device;
    }

    CommandQueues VulkanBuilder::GetQueues()
    {
        vkb::Device& Device = reinterpret_cast<_BuilderVKBStructs*>(_StructData.get())->Device;

        auto vkDevice = static_cast<vk::Device>(Device.device);

        CommandQueues queues = {};

        auto getQueue = [&](uint32_t index) -> vk::Queue { return vkDevice.getQueue(index, 0); };

        bool dedCompute = false;
        bool dedTransfer = false;

        for (uint32_t i = 0; i < Device.queue_families.size(); i++)
        {
            auto& queue = Device.queue_families[i];
            auto currentQueue = getQueue(i);

            if (static_cast<vk::QueueFlags>(queue.queueFlags) & vk::QueueFlagBits::eGraphics)
            {
                queues.GraphicsQueue = currentQueue;
                queues.GraphicsIndex = i;

                queues.PresentQueue = currentQueue;
                queues.PresentIndex = i;
            }

            if (static_cast<vk::QueueFlags>(queue.queueFlags) & vk::QueueFlagBits::eCompute)
            {
                if (dedCompute) // If we already have a dedicated compute queue, skip
                    continue;

                if (queues.GraphicsIndex == i)
                    dedCompute = false;
                else
                    dedCompute = true;

                queues.ComputeQueue = currentQueue;
                queues.ComputeIndex = i;
            }

            if (static_cast<vk::QueueFlags>(queue.queueFlags) & vk::QueueFlagBits::eTransfer)
            {
                if (dedTransfer)
                    continue;

                if (queues.GraphicsIndex == i || queues.ComputeIndex == i)
                    dedTransfer = false;
                else
                    dedTransfer = true;

                queues.TransferQueue = currentQueue;
                queues.TransferIndex = i;
            }
        }

        if (!queues.GraphicsQueue)
        {
            VULRAY_LOG_ERROR("No Graphics Queue Found");
            throw std::runtime_error("No Graphics Queue Found");
        }

        if (DedicatedCompute && !dedCompute)
        {
            VULRAY_LOG_ERROR("No Compute Queue Found");
            throw std::runtime_error("No Compute Queue Found");
        }

        if (DedicatedTransfer && !dedTransfer)
        {
            VULRAY_LOG_ERROR("No Transfer Queue Found");
            throw std::runtime_error("No Transfer Queue Found");
        }

        return queues;
    }

    // vr::Instance
    void InstanceWrapper::DestroyInstance(vk::Instance instance, vk::DebugUtilsMessengerEXT messenger)
    {

        if (instance)
        {
            if (messenger)
            {
                auto vkDestroyDebugUtilsMessengerEXT_ = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
                vkDestroyDebugUtilsMessengerEXT_(instance, messenger, nullptr);
            }
            instance.destroy();
            return;
        }
        VULRAY_LOG_WARNING("Called Instance::DestroyInstance with invalid vk::Instance");
    }
    void InstanceWrapper::DestroyInstance(InstanceWrapper instance)
    {

        InstanceWrapper::DestroyInstance(instance.InstanceHandle, instance.DebugMessenger);
    }

    SwapchainBuilder::SwapchainBuilder(vk::Device dev, vk::PhysicalDevice physdev, vk::SurfaceKHR surf,
                                       uint32_t gfxQueueidx, uint32_t presentQueueidx)
        : Device(dev), PhysicalDevice(physdev), Surface(surf), GraphicsQueueIndex(gfxQueueidx),
          PresentQueueIndex(presentQueueidx)
    {
    }

    SwapchainResources SwapchainBuilder::BuildSwapchain(vk::SwapchainKHR oldswapchain)
    {
        assert(Height != 0);
        assert(Width != 0);
        assert(Device);
        assert(PhysicalDevice);
        assert(GraphicsQueueIndex != UINT32_MAX);
        assert(PresentQueueIndex != UINT32_MAX);
        assert(PhysicalDevice);
        assert(Surface);

        std::vector<vk::SurfaceFormatKHR> surfaceFormats = PhysicalDevice.getSurfaceFormatsKHR(Surface);
        vk::SurfaceFormatKHR compatibleFormat = {};
        bool foundSurfaceFormat = false;
        // See if a compatible format is found, we don't guarantee the colorspace
        // will be the same, but we try to find the desired format
        for (auto format : surfaceFormats)
        {
            // if it fulfills our criteria
            if (format.format == DesiredFormat && format.colorSpace == ColorSpace)
            {
                compatibleFormat = format;
                foundSurfaceFormat = true;
                break;
            }

            if (format.format == DesiredFormat)
            {
                compatibleFormat = format;
                foundSurfaceFormat = true;
            }
        }

        if (compatibleFormat.format != DesiredFormat)
            VULRAY_LOG_ERROR("Desired Format is not available, Fallback is vk::Format::eB8G8R8A8Srgb");
        if (compatibleFormat.colorSpace != ColorSpace)
            VULRAY_FLOG_ERROR("Desired ColorSpace is not available, Using: %s",
                              vk::to_string(compatibleFormat.colorSpace));

        vkb::SwapchainBuilder* swapBuilder = nullptr;

        swapBuilder = new vkb::SwapchainBuilder(PhysicalDevice, Device, Surface, GraphicsQueueIndex, PresentQueueIndex);

        if (oldswapchain)
            swapBuilder->set_old_swapchain(oldswapchain);
        if (foundSurfaceFormat)
        {
            swapBuilder->set_desired_format(compatibleFormat);
        }
        else
        {
            swapBuilder->use_default_format_selection();
        }
        swapBuilder->set_desired_extent(Width, Height)
            .set_desired_present_mode((VkPresentModeKHR)PresentMode)
            .add_fallback_present_mode((VkPresentModeKHR)vk::PresentModeKHR::eMailbox)
            .set_desired_min_image_count(BackBufferCount) /*copy raytracing texture*/
            .set_image_usage_flags((VkImageUsageFlags)(ImageUsage | vk::ImageUsageFlagBits::eColorAttachment));

        auto buildResult = swapBuilder->build();
        if (!buildResult.has_value())
        {
            VULRAY_FLOG_ERROR("Swapchain Build failed, Error: %s", buildResult.error().message());
            throw std::runtime_error("Swapchain Build failed");
            return {};
        }

        auto swapchain = buildResult.value();

        auto swapImageViews = swapchain.get_image_views();
        auto swapImages = swapchain.get_images();

        if (!swapImages.has_value())
            VULRAY_FLOG_ERROR("Swapchain Images Error: %s", swapImages.error().message());
        if (!swapImageViews.has_value())
            VULRAY_FLOG_ERROR("Swapchain Image Views Error: %s", swapImageViews.error().message());

        auto images = *reinterpret_cast<std::vector<vk::Image>*>(&swapImages.value());
        auto imageviews = *reinterpret_cast<std::vector<vk::ImageView>*>(&swapImageViews.value());
        return {swapchain.swapchain, images, imageviews, (vk::Format)swapchain.image_format,
                (vk::Extent2D)swapchain.extent};
    }

    void SwapchainBuilder::DestroySwapchain(vk::Device device, const SwapchainResources& res)
    {
        device.destroySwapchainKHR(res.SwapchainHandle);

        for (auto i : res.SwapchainImageViews) device.destroyImageView(i);
    }
    void SwapchainBuilder::DestroySwapchainResources(vk::Device device, const SwapchainResources& res)
    {
        for (auto i : res.SwapchainImageViews) device.destroyImageView(i);
    }
} // namespace vr

static VKAPI_ATTR VkBool32 VKAPI_CALL VulrayVulkanDebugCback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                             void* pUserData)
{
    std::string msgHead = "[Vulkan][";
    const char* msgSeverity = vkb::to_string_message_severity(messageSeverity);
    const char* msgType = vkb::to_string_message_type(messageType);
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        VULRAY_FLOG_VERBOSE("[Vulkan][%s][%s]: %s", msgType, msgSeverity, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        VULRAY_FLOG_INFO("[Vulkan][%s][%s]: %s", msgType, msgSeverity, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        VULRAY_FLOG_WARNING("[Vulkan][%s][%s]: %s", msgType, msgSeverity, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        VULRAY_FLOG_ERROR("[Vulkan][%s][%s]: %s", msgType, msgSeverity, pCallbackData->pMessage);
        break;
    }

    return VK_FALSE;
}
