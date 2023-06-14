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

            MedianDenoiser(vk::Device device, uint32_t height, uint32_t width)
                : DenoiserInterface(device, height, width) {}

            MedianDenoiser() = delete;
            MedianDenoiser(const MedianDenoiser&) = delete;
            ~MedianDenoiser() override = default;
            

            std::vector<Resource> GetRequiredResources() override;

            void Initialize(std::vector<Resource>& resources) override;          
        };
    }
}