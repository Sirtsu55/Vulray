#include "Vulray/Denoisers/DenoiserInterface.h"

namespace vr
{

    namespace Denoise
    {

        DenoiserInterface::~DenoiserInterface()
        {
            auto vulkanDevice = mDevice->GetDevice();

            for (auto& img : mImages)
                mDevice->DestroyImage(img);

            // lambda to destroy image views and samplers
            auto destroyImg = [vulkanDevice](Resource& resource)
            {
                vulkanDevice.destroyImageView(resource.View);
                vulkanDevice.destroySampler(resource.Sampler);
            };

            for(auto& resource : mInputResources)
                destroyImg(resource);
            for(auto& resource : mOutputResources)
                destroyImg(resource);
            for(auto& resource : mInternalResources)
                destroyImg(resource);
        }
        void DenoiserInterface::CreateResources(std::vector<Resource> &resources)
        {
            auto vulkanDevice = mDevice->GetDevice();
                
            mImages.resize(2);

            auto maxAnisotropy = mDevice->GetProperties().limits.maxSamplerAnisotropy;

            // ALl resources are images
            auto imageInfo = vk::ImageCreateInfo()
                .setImageType(vk::ImageType::e2D)
                .setFormat(vk::Format::eUndefined) // Format depends on the resource type
                .setExtent(vk::Extent3D(mWidth, mHeight, 1))
                .setMipLevels(1)
                .setArrayLayers(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setTiling(vk::ImageTiling::eOptimal)
                .setUsage(vk::ImageUsageFlagBits::eSampled)
                .setSharingMode(vk::SharingMode::eExclusive)
                .setInitialLayout(vk::ImageLayout::eUndefined);
                
            for (auto& resource : resources)
            {
                imageInfo.format = resource.Format;
                imageInfo.usage = resource.Usage;

                if((int)resource.Type & 0b10) // if second bit is set, so it's an output
                    imageInfo.usage |= vk::ImageUsageFlagBits::eStorage;

                auto& img = mImages.emplace_back(mDevice->CreateImage(imageInfo, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT));

                resource.Image = img.Image; // Set the image created to the resource

                // Create Image View
                auto viewInfo = vk::ImageViewCreateInfo()
                    .setImage(img.Image)
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(imageInfo.format)
                    .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
                resource.View = vulkanDevice.createImageView(viewInfo);

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
                    .setCompareOp(vk::CompareOp::eAlways)
                    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                    .setMipLodBias(0.0f)
                    .setMinLod(0.0f)
                    .setMaxLod(1.0f);
                resource.Sampler = vulkanDevice.createSampler(samplerInfo);

                // Add the resource to the correct vector
                if((int)resource.Type & 0b01) // if first bit is set, so it's an input
                    mInputResources.push_back(resource);
                else if((int)resource.Type & 0b10) // if second bit is set, so it's an output
                    mOutputResources.push_back(resource);
                else
                    mInternalResources.push_back(resource);
            }


        }
    }

}