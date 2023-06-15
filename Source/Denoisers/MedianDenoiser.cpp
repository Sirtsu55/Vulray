
#include "Vulray/Denoisers/MedianDenoiser.h"

#include "MedianDenoiser.spv.h"

namespace vr
{
    namespace Denoise
    {
        MedianDenoiser::MedianDenoiser(vr::VulrayDevice* device, uint32_t width, uint32_t height)
            : DenoiserInterface(device, width, height)
        {
        }

        void MedianDenoiser::Initialize(vk::ImageUsageFlags inputUsage, vk::ImageUsageFlags outputUsage)
        {
            // Create the resources
            auto resources = GetRequiredResources();
            DenoiserInterface::CreateResources(resources, inputUsage, outputUsage);

            mDescriptorItems = {
                // Input image
                vr::DescriptorItem(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute, 1),
                // Output image
                vr::DescriptorItem(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 1)
            };

            DenoiserInterface::CreateDescriptorBuffer();
        }

        std::vector<Resource> MedianDenoiser::GetRequiredResources()
        {
            std::vector<Resource> resources(2);
            resources[0].Type = ResourceType::InputGeneral; // Median denoiser can be used in any context
            resources[0].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[0].Usage = vk::ImageUsageFlagBits::eSampled; // Want to sample the input image
            

            resources[1].Type = ResourceType::OutputFinal;
            resources[1].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[1].Usage = vk::ImageUsageFlagBits::eStorage; // Storage image for the output
            return resources;
        }
    }
}
