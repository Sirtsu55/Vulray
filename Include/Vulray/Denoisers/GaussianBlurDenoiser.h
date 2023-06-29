#pragma once

#include "Vulray/Denoisers/DenoiserInterface.h"

// SPIR-V Compiled shader code

namespace vr
{
    namespace Denoise
    {

        class GaussianBlurDenoiser : public DenoiserInterface
        {
          public:
            GaussianBlurDenoiser(vr::VulrayDevice* device, uint32_t width, uint32_t height);
            ~GaussianBlurDenoiser() override;

            GaussianBlurDenoiser() = delete;
            GaussianBlurDenoiser(const GaussianBlurDenoiser&) = delete;

            // Override the base class functions

            void Initialize(vk::ImageUsageFlags inputUsage = (vk::ImageUsageFlags)0,
                            vk::ImageUsageFlags outputUsage = (vk::ImageUsageFlags)0) override;

            std::vector<Resource> GetRequiredResources() override;

            void Denoise(vk::CommandBuffer cmdBuffer) override;

          private:
            std::vector<DescriptorItem> mDescriptorItems = {};
            vk::DescriptorSetLayout mDescriptorSetLayout = nullptr;
            DescriptorBuffer mDescriptorBuffer = {};

            vk::Pipeline mPipeline = nullptr;
            vk::PipelineLayout mPipelineLayout = nullptr;

            vk::ShaderModule mShaderModule = nullptr;

          public:
            /// @brief The settings for the gaussian blur
            struct Settings
            {
                /// @brief The radius of the gaussian blur
                uint32_t Radius = 3;

                /// @brief The sigma value for the gaussian blur, smoothness
                float Sigma = 1.0f;
            };

          private:
            // Data for the push constants
            struct PushConstantData
            {
                uint32_t Width;
                uint32_t Height;
                Settings DenoiserSettings;
            };
        };
    } // namespace Denoise
} // namespace vr