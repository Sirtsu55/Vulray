
#include "Vulray/Denoisers/MedianDenoiser.h"

namespace vr
{
    namespace Denoise
    {

        std::vector<Resource> MedianDenoiser::GetRequiredResources()
        {
            std::vector<Resource> resources(2);
            resources[0].Type = ResourceType::GeneralInput; // Median denoiser can be used in any context
            resources[0].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[0].Usage = vk::ImageUsageFlagBits::eSampled;
            

            resources[1].Type = ResourceType::Output;
            resources[1].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[1].Usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

            return resources;
        }

        void MedianDenoiser::Initialize(std::vector<Resource> &resources)
        {
            DenoiserInterface::Initialize(resources);

            // Create the descriptor set layout
            std::vector<vr::DescriptorItem> descriptorItems(2);
            descriptorItems[0] = vr::DescriptorItem(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute, 1, &mResources[0].Image);
            descriptorItems[1] = vr::DescriptorItem(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 1, &mResources[1].Image);

        }
    }
}
