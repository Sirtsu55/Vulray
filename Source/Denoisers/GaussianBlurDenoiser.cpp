
#include "Vulray/Denoisers/GaussianBlurDenoiser.h"

#include "GaussianBlurDenoiser.spv.h"

namespace vr
{
    namespace Denoise
    {
        GaussianBlurDenoiser::GaussianBlurDenoiser(vr::VulrayDevice* device, const DenoiserSettings& settings)
            : DenoiserInterface(device, settings)
        {
            mDenoiserParams = new Parameters();

            Init();
        }

        GaussianBlurDenoiser::~GaussianBlurDenoiser()
        {
            // Destroy descriptor set layout
            mDevice->GetDevice().destroyDescriptorSetLayout(mDescriptorSetLayout);
            mDevice->DestroyBuffer(mDescriptorBuffer.Buffer);

            // Destroy pipeline
            mDevice->GetDevice().destroyPipeline(mPipeline);
            mDevice->GetDevice().destroyPipelineLayout(mPipelineLayout);

            mDevice->GetDevice().destroyShaderModule(mShaderModule);

            delete (Parameters*)mDenoiserParams;
        }

        void GaussianBlurDenoiser::Init()
        {
            // Create the resources
            auto resources = GetRequiredResources();
            DenoiserInterface::CreateResources(resources, mSettings.InputUsage, mSettings.OutputUsage);

            mDescriptorItems = {
                // Input image
                vr::DescriptorItem(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute, 1,
                                   &mInputResources[0].AccessImage),
                // Output image
                vr::DescriptorItem(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 1,
                                   &mOutputResources[0].AccessImage)};

            mDescriptorSetLayout = mDevice->CreateDescriptorSetLayout(mDescriptorItems);
            mDescriptorBuffer = mDevice->CreateDescriptorBuffer(mDescriptorSetLayout, mDescriptorItems,
                                                                vr::DescriptorBufferType::Combined);

            mDevice->UpdateDescriptorBuffer(mDescriptorBuffer, mDescriptorItems, vr::DescriptorBufferType::Combined);

            // Create the pipeline layout
            auto pushConstantRange = vk::PushConstantRange()
                                         .setStageFlags(vk::ShaderStageFlagBits::eCompute)
                                         .setOffset(0)
                                         .setSize(sizeof(PushConstantData));
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
                                                                  .setSetLayoutCount(1)
                                                                  .setPSetLayouts(&mDescriptorSetLayout)
                                                                  .setPPushConstantRanges(&pushConstantRange)
                                                                  .setPushConstantRangeCount(1);

            mPipelineLayout = mDevice->GetDevice().createPipelineLayout(pipelineLayoutInfo);

            // From char array to uint32_t array
            uint32_t* shaderCode = (uint32_t*)&g_GaussianBlurDenoiser_main;

            // Spirv always has a size that is a multiple of 4
            uint32_t spvSize = sizeof(g_GaussianBlurDenoiser_main);

            // Create the shader module
            auto shaderModuleInfo = vk::ShaderModuleCreateInfo().setCodeSize(spvSize).setPCode(shaderCode);
            mShaderModule = mDevice->GetDevice().createShaderModule(shaderModuleInfo);

            //  Create the pipeline
            auto pipelineInfo = vk::ComputePipelineCreateInfo()
                                    .setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT)
                                    .setLayout(mPipelineLayout)
                                    .setStage(vk::PipelineShaderStageCreateInfo()
                                                  .setStage(vk::ShaderStageFlagBits::eCompute)
                                                  .setModule(mShaderModule)
                                                  .setPName("GaussianBlurDenoiser_main"));

            auto res = mDevice->GetDevice().createComputePipeline(nullptr, pipelineInfo);

            if (res.result != vk::Result::eSuccess)
                VULRAY_LOG_ERROR("Failed to create median denoiser pipeline");
            mPipeline = res.value;
        }

        std::vector<Resource> GaussianBlurDenoiser::GetRequiredResources()
        {
            std::vector<Resource> resources(2);
            resources[0].Type = ResourceType::InputGeneral; // Median denoiser can be used in any context
            resources[0].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[0].Usage = vk::ImageUsageFlagBits::eSampled;       // Want to sample the input image
            resources[0].AccessImage.Layout = vk::ImageLayout::eGeneral; // General layout for the output image

            resources[1].Type = ResourceType::OutputFinal;
            resources[1].Format = vk::Format::eR32G32B32A32Sfloat;
            resources[1].Usage = vk::ImageUsageFlagBits::eStorage;       // Storage image for the output
            resources[1].AccessImage.Layout = vk::ImageLayout::eGeneral; // General layout for the output image
            return resources;
        }

        void GaussianBlurDenoiser::Denoise(vk::CommandBuffer cmdBuffer)
        {
            // Bind the pipeline
            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, mPipeline);
            mDevice->BindDescriptorBuffer({mDescriptorBuffer}, cmdBuffer);
            mDevice->BindDescriptorSet(mPipelineLayout, 0, 0, 0, cmdBuffer, vk::PipelineBindPoint::eCompute);

            PushConstantData pushData;
            pushData.Params.Radius = ((Parameters*)mDenoiserParams)->Radius;
            pushData.Params.Sigma = ((Parameters*)mDenoiserParams)->Sigma;
            pushData.Width = mSettings.Width;
            pushData.Height = mSettings.Height;

            // Push constants
            cmdBuffer.pushConstants(mPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConstantData),
                                    &pushData);

            cmdBuffer.dispatch(mSettings.Width / 16, mSettings.Height / 16, 1);
        }
    } // namespace Denoise
} // namespace vr
