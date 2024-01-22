#pragma once

// Include standard headers.
#include <cassert>

// Define the assert macro.
#define VULRAY_ASSERT(x) assert(x)

// Platform specific includes.

#ifdef _WIN32 // Windows support.

#define VK_USE_PLATFORM_WIN32_KHR

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace vr
{
    using WindowHandle = HWND;
}

#elif __linux__ // Linux support.

/// @todo Linux support.

#endif // _WIN32

// Include Vulkan headers.
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#include <vulkan/vulkan.hpp>

// Include the Vulkan bootstrap header.
#include <VkBootstrap.h>

// Include the Vulkan memory allocator header.
#include <vk_mem_alloc.h>

namespace vr
{
#if defined(VULRAY_CUSTOM_SHARED_PTR_CLASS) && defined(VULRAY_CUSTOM_SHARED_PTR_FUNC)
    template <typename T>
    using Handle = VULRAY_CUSTOM_SHARED_PTR_CLASS<T>;

    template <typename T, typename... Args>
    constexpr Handle<T> MakeHandle(Args&&... args)
    {
        return VULRAY_CUSTOM_SHARED_PTR_FUNC<T>(std::forward<Args>(args)...);
    }
#else
    template <typename T>
    using Handle = std::shared_ptr<T>;

    template <typename T, typename... Args>
    constexpr Handle<T> MakeHandle(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
#endif // defined(VULRAY_CUSTOM_SHARED_PTR_CLASS) && defined(VULRAY_CUSTOM_SHARED_PTR_FUNC)

    template <typename T>
    using Vector = std::vector<T>;

    /// @brief CommandBuffer is a class that encapsulates a command buffer.
    class CommandBuffer
    {
    public:
        /// @brief Constructor.
        /// @param device Logical device.
        /// @param commandPool Command pool.
        CommandBuffer(vk::Device device, vk::CommandPool commandPool);

        /// @brief Destructor.
        ~CommandBuffer() { mDevice.freeCommandBuffers(mCommandPool, mCommandBuffer); }

    private:
        /// @brief Logical device.
        vk::Device mDevice = nullptr;

        /// @brief Command pool.
        vk::CommandPool mCommandPool = nullptr;

        /// @brief Command buffer.
        vk::CommandBuffer mCommandBuffer = nullptr;
    };

    class CommandAllocator
    {
    public:
        /// @brief Constructor.
        /// @param device Logical device.
        /// @param queue Queue.
        CommandAllocator(vk::Device device, vk::Queue queue);

        /// @brief Destructor.
        ~CommandAllocator() { mDevice.destroyCommandPool(mCommandPool); }

        /// @brief Get the command pool.
        /// @return Command pool.
        vk::CommandPool GetCommandPool() const { return mCommandPool; }

        /// @brief Create a command buffer.
        /// @return Command buffer.
        Handle<CommandBuffer> CreateCommandBuffer();

        /// @brief Reset the command pool.
        void Reset();

    private:
        /// @brief Logical device.
        vk::Device mDevice = nullptr;

        /// @brief Command pool.
        vk::CommandPool mCommandPool = nullptr;
    };

    /// @brief CommandQueue is a class that encapsulates a command queue.
    class CommandQueue
    {
    public:
        /// @brief Constructor.
        /// @param queue Queue.
        /// @param queueType Queue type.
        CommandQueue(vk::Queue queue, vk::QueueFlags queueType);

        /// @brief Destructor.
        ~CommandQueue();

        /// @brief Get the queue type.
        /// @return Queue type.
        vk::QueueFlagBits GetQueueType() const { return mQueueType; }

        /// @brief Wait for the queue to be idle.
        void WaitIdle() { mQueue.waitIdle(); }

        /// @brief Submit command buffers to the queue.
        void Submit(const Vector<Handle<CommandBuffer>>& commandBuffers);

        /// @brief Queue a wait semaphore to wait on next submit.
        /// @param semaphore Semaphore.
        /// @param value Value.
        void Wait(vk::Semaphore semaphore, uint64_t value)
        {
            mWaitSemaphores.emplace_back(semaphore);
            mWaitSemaphoreValues.emplace_back(value);
        }

        /// @brief Queue a signal semaphore to signal on next submit.
        /// @param semaphore Semaphore.
        /// @param value Value.
        void Signal(vk::Semaphore semaphore, uint64_t value)
        {
            mSignalSemaphores.emplace_back(semaphore);
            mSignalSemaphoreValues.emplace_back(value);
        }

        /// @brief Get the queue.
        /// @return Queue.
        vk::Queue GetQueue() const { return mQueue; }

    private:
        /// @brief Logical device.
        vk::Device mDevice = nullptr;

        /// @brief Queue.
        vk::Queue mQueue = nullptr;

        /// @brief Queue type.
        vk::QueueFlagBits mQueueType = (vk::QueueFlagBits)0;

        /// @brief Wait semaphores.
        Vector<vk::Semaphore> mWaitSemaphores = {};

        /// @brief Wait semaphore values.
        Vector<uint64_t> mWaitSemaphoreValues = {};

        /// @brief Signal semaphores.
        Vector<vk::Semaphore> mSignalSemaphores = {};

        /// @brief Signal semaphore values.
        Vector<uint64_t> mSignalSemaphoreValues = {};
    };

    /// @brief Semaphore class encapsulates a fence. This corresponds to a timeline semaphore which is just better
    /// than binary semaphores / fences. Only area where binary semaphores are needed is for swapchain image
    /// acquisition, but that is handled by the swapchain class.
    class Semaphore
    {
    public:
        /// @brief Constructor.
        /// @param device Logical device.
        /// @param semaphore Semaphore.
        Semaphore(vk::Device device);

        /// @brief Destructor.
        ~Semaphore() { mDevice.destroySemaphore(mSemaphore); }

        /// @brief Get the semaphore.
        /// @return Semaphore.
        vk::Semaphore GetSemaphore() const { return mSemaphore; }

    private:
        /// @brief Logical device.
        vk::Device mDevice = nullptr;

        /// @brief Fence.
        vk::Semaphore mSemaphore = nullptr;
    };

    /// @brief Swapchain class encapsulates a swapchain.
    class Swapchain
    {
    public:
        Swapchain(vk::Device device, WindowHandle windowHandle, vk::Extent2D extent, vk::Format format,
                  vk::PresentModeKHR presentMode);

        /// @brief Destructor.
        ~Swapchain();

        /// @brief Get the swapchain.
        /// @return Swapchain.
        vk::SwapchainKHR GetSwapchain() const { return mSwapchain; }

    private:
        /// @brief Logical device.
        vk::Device mDevice = nullptr;

        /// @brief Swapchain.
        vk::SwapchainKHR mSwapchain = nullptr;

        /// @brief Swapchain images.
        std::vector<vk::Image> mImages;

        /// @brief Swapchain image views.
        std::vector<vk::ImageView> mImageViews;

        /// @brief Present queue.
        vk::Queue mPresentQueue = nullptr;

        /// @brief Present semaphore.
        vk::Semaphore mPresentSemaphore = nullptr;

        /// @brief Render semaphore.
        vk::Semaphore mRenderSemaphore = nullptr;

        /// @brief Render fence.
        vk::Fence mRenderFence = nullptr;
    };

    /// @brief DeviceBuilder is a class that encapsulates a device builder.
    struct DeviceRequirements
    {
        /// @brief Enable validation or not. Enabling this will enable the debug messenger.
        bool EnableValidation = false;

        /// @brief Enable debug messenger or not.
        bool EnableDebugMessenger = false;

        /// @brief Message severities to enable.
        vk::DebugUtilsMessageSeverityFlagsEXT MessageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;

        /// @brief Debug callback.
        PFN_vkDebugUtilsMessengerCallbackEXT DebugCallback = nullptr;

        /// @brief Debug callback user data.
        void* DebugCallbackUserData = nullptr;

        /// @brief Instance extensions to enable. All surface extensions are enabled by default.
        Vector<const char*> InstanceExtensions = {};

        /// @brief Instance layers to enable.
        Vector<const char*> InstanceLayers = {};

        /// @brief Additional device extensions apart from the required ones of vulray.
        std::vector<const char*> DeviceExtensions = {};

        /// @brief Vulkan 1.0 features.
        vk::PhysicalDeviceFeatures PhysicalDeviceFeatures10 = {};

        /// @brief Vulkan 1.1 features.
        vk::PhysicalDeviceVulkan11Features PhysicalDeviceFeatures11 = {};

        /// @brief Vulkan 1.2 features.
        vk::PhysicalDeviceVulkan12Features PhysicalDeviceFeatures12 = {};

        /// @brief Vulkan 1.3 features.
        vk::PhysicalDeviceVulkan13Features PhysicalDeviceFeatures13 = {};

        /// @brief Feature pNext chain. If your own extension requires a feature, you can add the start of the chain
        /// here.
        void* FeatureChain = nullptr;

        /// @brief Bool to indicate if you want a dedicated compute queue.
        bool DedicatedCompute = false;

        /// @brief Bool to indicate if you want a dedicated transfer queue.
        bool DedicatedTransfer = false;

        /// @brief Separate compute and transfer queue from graphics queue. So compute and transfer can be same queue
        /// family, but separate from graphics queue family. Don't combine this with dedicated compute or transfer
        /// queue. May lead to undefined behavior.
        bool SeparateComputeTransferQueue = false;
    };

    /// @brief VulkanContext is a class that encapsulates a vulkan context.
    struct VulkanContext
    {
        vk::Instance Instance = nullptr;

        vk::DebugUtilsMessengerEXT DebugMessenger = nullptr;

        vk::Device Device = nullptr;

        vk::PhysicalDevice PhysicalDevice = nullptr;

        vk::Queue GraphicsQueue = nullptr;

        vk::Queue ComputeQueue = nullptr;

        vk::Queue TransferQueue = nullptr;

        vk::DispatchLoaderDynamic DynLoader = {};
    };

    /// @brief Create a vulkan context with the given requirements.
    /// @param req Device requirements.
    VulkanContext CreateVulkanContext(DeviceRequirements& req);

    /// @brief Destroy a vulkan context.
    /// @param context Vulkan context.
    void DestroyVulkanContext(VulkanContext& context);

    /// @brief Device is a class that encapsulates a graphics device.
    class Device
    {
    public:
        /// @brief Constructor with a vulkan context.
        Device(const VulkanContext& ctx);

        /// @brief Destructor.
        ~Device();

        /// @brief Create a swapchain.
        /// @param windowhHndle Handle to a native window.
        /// @param extent Extents of the swapchain.
        /// @param format Format of the swapchain.
        /// @param presentMode Present mode of the swapchain. Fallbacks to FIFO if the present mode is not supported.
        /// @return Swapchain.
        Handle<Swapchain> CreateSwapchain(WindowHandle windowHandle, vk::Extent2D extent, vk::Format format,
                                          vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo);

        void WaitIdle() { mCtx.Device.waitIdle(); }

    private:
        /// @brief Vulkan context.
        VulkanContext mCtx = {};

    private:
        /// @brief Check properties of a physical device.
        /// @param physicalDevice Physical device.
        /// @param extensions Required extensions.
        /// @return True if the physical device is suitable, false otherwise.
        bool IsPhysicalDeviceCompatible(vk::PhysicalDevice physicalDevice, const Vector<const char*>& extensions);
    };

} // namespace vr
