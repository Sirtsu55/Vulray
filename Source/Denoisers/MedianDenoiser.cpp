
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
            std::vector<vk::DescriptorSetLayoutBinding> bindings(2);
        }
    }
}
