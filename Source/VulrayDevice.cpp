#include "Vulray/VulrayDevice.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace vr
{
    VulrayDevice::VulrayDevice(vk::Instance inst, vk::Device dev, vk::PhysicalDevice physDev)
		: mInstance(inst), mDevice(dev), mPhysicalDevice(physDev)
    {

        mDynLoader.init(inst, vkGetInstanceProcAddr, dev, vkGetDeviceProcAddr);

        //Create allocator for accelstructs
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physDev;
        allocatorInfo.device = dev;
        allocatorInfo.instance = inst;
        allocatorInfo.flags = VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        vmaCreateAllocator(&allocatorInfo, &mVMAllocator);

        
        //Get ray tracing and accel properties
        auto deviceProperties = vk::PhysicalDeviceProperties2KHR();
        deviceProperties.pNext = &mRayTracingProperties;
        mRayTracingProperties.pNext = &mAccelProperties;
        mAccelProperties.pNext = &mDescriptorBufferProperties;
        mDescriptorBufferProperties.pNext = nullptr;

        mPhysicalDevice.getProperties2KHR(&deviceProperties, mDynLoader);

        mDeviceProperties = mPhysicalDevice.getProperties();
    }
    


    VulrayDevice::~VulrayDevice()
    {
        vmaDestroyAllocator(mVMAllocator);
    }

    vk::CommandBuffer VulrayDevice::CreateCommandBuffer(vk::CommandPool pool, vk::CommandBufferLevel level)
    {
        auto cmdBuf = vk::CommandBuffer();
        auto allocInfo = vk::CommandBufferAllocateInfo(pool, level, 1);
        auto _ = mDevice.allocateCommandBuffers(&allocInfo, &cmdBuf);
        return cmdBuf;
    }

    std::vector<vk::CommandBuffer> VulrayDevice::CreateCommandBuffers(vk::CommandPool pool, uint32_t count, vk::CommandBufferLevel level)
    {
        return mDevice.allocateCommandBuffers(vk::CommandBufferAllocateInfo(pool, level, count));
    }

#ifdef VULRAY_BUILD_DENOISERS
    std::vector<AllocatedImage> VulrayDevice::CreateDenoiserResources(
            std::vector<Denoise::Resource>& resources,
            uint32_t height,
            uint32_t width)
    {
        auto outImages = std::vector<AllocatedImage>(resources.size());

        // ALl resources are images
        auto imageInfo = vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(vk::Format::eUndefined) // Format depends on the resource type
            .setExtent(vk::Extent3D(width, height, 1))
            .setMipLevels(1)
            .setArrayLayers(1)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(vk::ImageUsageFlagBits::eSampled)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);
            
        for (auto& resource : resources)
        {
            auto resType = resource.GetType();
            switch (resType)
            {
            case Denoise::ResourceType::GeneralInput:
            {
                imageInfo.format = vk::Format::eR16G16B16A16Sfloat;
                break;
            }
            case Denoise::ResourceType::Output:
            {
                imageInfo.usage |= vk::ImageUsageFlagBits::eStorage;
                imageInfo.format = vk::Format::eR16G16B16A16Sfloat;
                break;
            }
            }
            auto& img = outImages.emplace_back(CreateImage(imageInfo, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT));

            // Create Image View
            auto viewInfo = vk::ImageViewCreateInfo()
                .setImage(img.Image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(imageInfo.format)
                .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            resource.Image.View = mDevice.createImageView(viewInfo);

            // Create Sampler
            mDeviceProperties.limits.maxSamplerAnisotropy;
            auto samplerInfo = vk::SamplerCreateInfo()
                .setMagFilter(vk::Filter::eLinear)
                .setMinFilter(vk::Filter::eLinear)
                .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                .setAnisotropyEnable(true)
                .setMaxAnisotropy(mDeviceProperties.limits.maxSamplerAnisotropy) // No performance hit
                .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                .setUnnormalizedCoordinates(false)
                .setCompareEnable(false)
                .setCompareOp(vk::CompareOp::eAlways)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0f)
                .setMinLod(0.0f)
                .setMaxLod(1.0f);
            resource.Image.Sampler = mDevice.createSampler(samplerInfo);
            resource.Image.Layout = vk::ImageLayout::eUndefined;
        }

        return outImages;
    }
#endif
}