#pragma once

#include "Vulray/Denoisers/DenoiserInterface.h"

// SPIR-V Compiled shader code

namespace vr
{
    namespace Denoise
    {

        class MedianDenoiser : public DenoiserInterface
        {
        public:

            MedianDenoiser(vr::VulrayDevice* device, uint32_t width, uint32_t height);

            MedianDenoiser() = delete;
            MedianDenoiser(const MedianDenoiser&) = delete;
            ~MedianDenoiser() override = default;
            
            
            void Initialize(vk::ImageUsageFlags inputUsage = (vk::ImageUsageFlags)0,
                vk::ImageUsageFlags outputUsage = (vk::ImageUsageFlags)0) override;

            std::vector<Resource> GetRequiredResources() override;

        };
    }
}