#include "Vulray/VulrayDevice.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace vr
{
    VulrayDevice::VulrayDevice(vk::Instance inst, vk::Device dev, vk::PhysicalDevice physDev, VmaAllocator allocator)
        : mInstance(inst), mDevice(dev), mPhysicalDevice(physDev), mVMAllocator(allocator)
    {

        mDynLoader.init(inst, vkGetInstanceProcAddr, dev, vkGetDeviceProcAddr);

        // Get ray tracing and accel properties
        auto deviceProperties = vk::PhysicalDeviceProperties2KHR();
        deviceProperties.pNext = &mRayTracingProperties;
        mRayTracingProperties.pNext = &mAccelProperties;
        mAccelProperties.pNext = &mDescriptorBufferProperties;
        mDescriptorBufferProperties.pNext = nullptr;

        mPhysicalDevice.getProperties2KHR(&deviceProperties, mDynLoader);

        mDeviceProperties = mPhysicalDevice.getProperties();

        // If the supplied allocator isn't null then return, because we don't need to create a new one
        if (mVMAllocator != nullptr)
        {
            mUserSuppliedAllocator = true;
            return;
        }

        // Create allocator for accelstructs
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physDev;
        allocatorInfo.device = dev;
        allocatorInfo.instance = inst;
        allocatorInfo.flags = VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        vmaCreateAllocator(&allocatorInfo, &mVMAllocator);
        mUserSuppliedAllocator = false;
    }

    VulrayDevice::~VulrayDevice()
    {
        if (!mUserSuppliedAllocator)
            vmaDestroyAllocator(mVMAllocator);
    }
} // namespace vr
