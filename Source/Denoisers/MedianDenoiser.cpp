
#include "Vulray/Denoisers/MedianDenoiser.h"

namespace vr
{
    namespace Denoise
    {

        MedianDenoiser::MedianDenoiser(vr::VulrayDevice* device, uint32_t width, uint32_t height)
            : DenoiserInterface(device, width, height)
        {
            // Create the resources
            auto resources = GetRequiredResources();
            CreateResources(resources);
        }

        std::vector<Resource> MedianDenoiser::GetRequiredResources()
        {
            std::vector<Resource> resources(2);
            resources[0].Type = ResourceType::InputGeneral; // Median denoiser can be used in any context
            resources[0].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[0].Usage = vk::ImageUsageFlagBits::eSampled;
            

            resources[1].Type = ResourceType::OutputFinal;
            resources[1].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[1].Usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

            return resources;
        }
    }
}
