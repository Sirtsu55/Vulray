#include "Vulray/Denoisers/DenoiserInterface.h"

namespace vr::Denoise
{
    DenoiserInterface::~DenoiserInterface()
    {
        auto vulkanDevice = mDevice->GetDevice();

        // lambda to destroy image views and samplers
        auto destroyImg = [this, vulkanDevice](Resource& resource) {
            vulkanDevice.destroyImageView(resource.AccessImage.View);
            vulkanDevice.destroySampler(resource.AccessImage.Sampler);
            mDevice->DestroyImage(resource.AllocImage);
        };

        for (auto& resource : mInputResources) destroyImg(resource);
        for (auto& resource : mOutputResources) destroyImg(resource);
        for (auto& resource : mInternalResources) destroyImg(resource);
    }
    void DenoiserInterface::CreateResources(std::vector<Resource>& resources, vk::ImageUsageFlags inputUsage,
                                            vk::ImageUsageFlags outputUsage)
    {
        auto vulkanDevice = mDevice->GetDevice();

        auto maxAnisotropy = mDevice->GetProperties().limits.maxSamplerAnisotropy;

        // ALl resources are images
        auto imageInfo = vk::ImageCreateInfo()
                             .setImageType(vk::ImageType::e2D)
                             .setFormat(vk::Format::eUndefined) // Format depends on the resource type
                             .setExtent(vk::Extent3D(mSettings.Width, mSettings.Height, 1))
                             .setMipLevels(1)
                             .setArrayLayers(1)
                             .setSamples(vk::SampleCountFlagBits::e1)
                             .setTiling(vk::ImageTiling::eOptimal)
                             .setSharingMode(vk::SharingMode::eExclusive)
                             .setInitialLayout(vk::ImageLayout::eUndefined);

        for (auto& resource : resources)
        {
            imageInfo.format = resource.Format;
            imageInfo.usage = resource.Usage | ((int)resource.Type & 0b01
                                                    ? inputUsage
                                                    : outputUsage); // set the usage depending on the type of resource

            resource.AllocImage = mDevice->CreateImage(imageInfo, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

            // Create Image View
            auto viewInfo =
                vk::ImageViewCreateInfo()
                    .setImage(resource.AllocImage.Image)
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(imageInfo.format)
                    .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            resource.AccessImage.View = vulkanDevice.createImageView(viewInfo);

            // Only create sampler if the resource is sampled
            if (resource.Usage & vk::ImageUsageFlagBits::eSampled)
            {
                // Create Sampler
                auto samplerInfo = vk::SamplerCreateInfo()
                                       .setMagFilter(vk::Filter::eLinear)
                                       .setMinFilter(vk::Filter::eLinear)
                                       .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                       .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                       .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                       .setAnisotropyEnable(true)
                                       .setMaxAnisotropy(maxAnisotropy) // No performance hit
                                       .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                                       .setUnnormalizedCoordinates(false)
                                       .setCompareEnable(false)
                                       .setCompareOp(vk::CompareOp::eNever)
                                       .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                       .setMipLodBias(0.0f)
                                       .setMinLod(0.0f)
                                       .setMaxLod(1.0f);
                resource.AccessImage.Sampler = vulkanDevice.createSampler(samplerInfo);
            }

            // Add the resource to the correct vector
            if ((int)resource.Type & 0b01) // if first bit is set, so it's an input
                mInputResources.push_back(resource);
            else if ((int)resource.Type & 0b10) // if second bit is set, so it's an output
                mOutputResources.push_back(resource);
            else
                mInternalResources.push_back(resource);
        }
    }

} // namespace vr::Denoise