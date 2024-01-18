#pragma once

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

// Set the extension for the surface creation function.
#ifdef _WIN32

#define VULRAY_VULKAN_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

#elif __linux__

#define VULRAY_VULKAN_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME

#endif // _WIN32

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
        /// @param dev Logical device.
        /// @param queueType Queue type.
        CommandQueue(vk::Device dev, vk::QueueFlagBits queueType);

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

    /// @brief Device is a class that encapsulates a graphics device.
    class Device
    {
    public:
        /// @brief Constructor.
        /// @param debugCallback Debug callback function pointer. If this is nullptr, no debug callback will be created
        /// and debug is disabled.
        /// @param severity Severity of the debug messages that will be reported.
        Device(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback = nullptr,
               vk::DebugUtilsMessageSeverityFlagsEXT severity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo);

        /// @brief Destructor.
        ~Device();

        /// @brief Get the logical device.
        /// @return Logical device.
        vk::Device GetDevice() const { return mDevice; }

        /// @brief Get the physical device.
        /// @return Physical device.
        vk::PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

        /// @brief Get the graphics queue.
        /// @return Graphics queue. No matter how many times this function is called, it is guaranteed to return the
        /// same queue.
        Handle<CommandQueue> GetGraphicsQueue() const { return mQueues.mGraphics; }

        /// @brief Get the compute queue.
        /// @return Compute queue. No matter how many times this function is called, it is guaranteed to return the
        /// same queue.
        Handle<CommandQueue> GetComputeQueue() const { return mQueues.mCompute; }

        /// @brief Get the transfer queue.
        /// @return Transfer queue. No matter how many times this function is called, it is guaranteed to return the
        /// same queue.
        Handle<CommandQueue> GetTransferQueue() const { return mQueues.mTransfer; }

        /// @brief Create a swapchain.
        /// @param windowhHndle Handle to a native window.
        /// @param extent Extents of the swapchain.
        /// @param format Format of the swapchain.
        /// @param presentMode Present mode of the swapchain. Fallbacks to FIFO if the present mode is not supported.
        /// @return Swapchain.
        Handle<Swapchain> CreateSwapchain(WindowHandle windowHandle, vk::Extent2D extent, vk::Format format,
                                          vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo);

    private:
        /// @brief Vulkan instance.
        vk::Instance mInstance = nullptr;

        /// @brief Vulkan debug messenger.
        vk::DebugUtilsMessengerEXT mDebugMessenger = nullptr;

        /// @brief Vulkan dispatch loader dynamic.
        vk::DispatchLoaderDynamic mDynLoader = {};

        /// @brief Logical device.
        vk::Device mDevice = nullptr;

        /// @brief Physical device.
        vk::PhysicalDevice mPhysicalDevice = nullptr;

        struct
        {
            /// @brief Graphics queue.
            Handle<CommandQueue> mGraphics = nullptr;

            /// @brief Compute queue.
            Handle<CommandQueue> mCompute = nullptr;

            /// @brief Transfer queue.
            Handle<CommandQueue> mTransfer = nullptr;

        } mQueues;

    private:
        /// @brief Check properties of a physical device.
        /// @param physicalDevice Physical device.
        /// @return True if the physical device is suitable, false otherwise.
        bool CheckPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice);
    };

} // namespace vr
