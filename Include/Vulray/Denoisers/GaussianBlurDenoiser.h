#pragma once

#include "Vulray/Denoisers/DenoiserInterface.h"

// SPIR-V Compiled shader code

namespace vr::Denoise
{
    class GaussianBlurDenoiser : public DenoiserInterface
    {
      public:
        GaussianBlurDenoiser(vr::VulrayDevice* device, const DenoiserSettings& settings);
        ~GaussianBlurDenoiser() override;

        GaussianBlurDenoiser() = delete;
        GaussianBlurDenoiser(const GaussianBlurDenoiser&) = delete;

        std::vector<Resource> GetRequiredResources() override;

        void Denoise(vk::CommandBuffer cmdBuffer) override;

      private:
        void Init();

        std::vector<DescriptorItem> mDescriptorItems = {};
        vk::DescriptorSetLayout mDescriptorSetLayout = nullptr;
        DescriptorBuffer mDescriptorBuffer = {};

        vk::Pipeline mPipeline = nullptr;
        vk::PipelineLayout mPipelineLayout = nullptr;

        vk::ShaderModule mShaderModule = nullptr;

      public:
        /// @brief The settings for the gaussian blur
        struct Parameters
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
            Parameters Params;
        };
    };
} // namespace vr::Denoise