#include "Vulray/VulrayDevice.h"
#include "Vulray/Shader.h"

#include <numeric>

static void ErrorCheckShader(const char* shaderName, const vr::Shader& sdr, vk::ShaderStageFlagBits state);

namespace vr
{

    vk::Pipeline VulrayDevice::CreateRayTracingPipeline(vk::PipelineLayout layout, const ShaderBindingTable &info, uint32_t recursuionDepth)
    {
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
        shaderStages.reserve(1 + info.MissShaders.size() + info.HitGroups.size() + info.CallableShaders.size());
        shaderGroups.reserve(1 + info.MissShaders.size() + info.HitGroups.size() + info.CallableShaders.size());
        
        //create ray gen shader groups
        {
            ErrorCheckShader("RayGen", info.RayGenShader, vk::ShaderStageFlagBits::eRaygenKHR);

            shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                .setStage(info.RayGenShader.Stage)
                .setModule(info.RayGenShader.Module)
                .setPName(info.RayGenShader.EntryPoint));
            
            uint32_t rayGenIndex = static_cast<uint32_t>(shaderStages.size() - 1);

            shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
                .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                .setGeneralShader(rayGenIndex)
                .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                .setIntersectionShader(VK_SHADER_UNUSED_KHR));
        }
        //create miss shader groups
        for(auto& shader : info.MissShaders)
        {
            ErrorCheckShader("Miss", shader, vk::ShaderStageFlagBits::eMissKHR);

            shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                .setStage(shader.Stage)
                .setModule(shader.Module)
                .setPName(shader.EntryPoint));

            uint32_t missIndex = static_cast<uint32_t>(shaderStages.size() - 1);
            
            shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
                .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                .setGeneralShader(missIndex)
                .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                .setIntersectionShader(VK_SHADER_UNUSED_KHR));
        }
        //create hit group shader groups
        for(auto& hg : info.HitGroups)
        {
            // null init hit group, will be filled in later
            auto hitGroup = vk::RayTracingShaderGroupCreateInfoKHR()
                .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
                .setGeneralShader(VK_SHADER_UNUSED_KHR)
                .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                .setIntersectionShader(VK_SHADER_UNUSED_KHR);

            //check if both shaders are null
            if(!hg.ClosestHitShader.Module && !hg.AnyHitShader.Module && !hg.IntersectionShader.Module)
            {
                VULRAY_LOG_ERROR("CreateRayTracingPipeline: Hit group must have at least one shader");
            }
            
            //add closest hit shader if it exists
            if(hg.ClosestHitShader.Module)
            {
                ErrorCheckShader("ClosestHit", hg.ClosestHitShader, vk::ShaderStageFlagBits::eClosestHitKHR);
                
                shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                    .setStage(hg.ClosestHitShader.Stage)
                    .setModule(hg.ClosestHitShader.Module)
                    .setPName(hg.ClosestHitShader.EntryPoint));

                uint32_t closestHitIndex = static_cast<uint32_t>(shaderStages.size() - 1);
                hitGroup.setClosestHitShader(closestHitIndex);
            }
            //add any hit shader if it exists
            if(hg.AnyHitShader.Module)
            {
                ErrorCheckShader("AnyHit", hg.AnyHitShader, vk::ShaderStageFlagBits::eAnyHitKHR);

                shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                    .setStage(hg.AnyHitShader.Stage)
                    .setModule(hg.AnyHitShader.Module)
                    .setPName(hg.AnyHitShader.EntryPoint));
                
                uint32_t anyHitIndex = static_cast<uint32_t>(shaderStages.size() - 1);
                hitGroup.setAnyHitShader(anyHitIndex);
            }
            //add intersection shader if it exists
            if(hg.IntersectionShader.Module)
            {
                ErrorCheckShader("Intersection", hg.IntersectionShader, vk::ShaderStageFlagBits::eIntersectionKHR);

                shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                    .setStage(hg.IntersectionShader.Stage)
                    .setModule(hg.IntersectionShader.Module)
                    .setPName(hg.IntersectionShader.EntryPoint));
                
                uint32_t intersectionIndex = static_cast<uint32_t>(shaderStages.size() - 1);
                hitGroup.setIntersectionShader(intersectionIndex);
            }

            shaderGroups.push_back(hitGroup);
        }
        //create callable shader groups
        for(auto& shader : info.CallableShaders)
        {
            ErrorCheckShader("Callable", shader, vk::ShaderStageFlagBits::eCallableKHR);

            shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                .setStage(shader.Stage)
                .setModule(shader.Module)
                .setPName(shader.EntryPoint));

            uint32_t callIndex = static_cast<uint32_t>(shaderStages.size() - 1);
            
            shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
                .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                .setGeneralShader(callIndex)
                .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                .setIntersectionShader(VK_SHADER_UNUSED_KHR));
        }

        if(recursuionDepth >= mRayTracingProperties.maxRayRecursionDepth)
        {
            VULRAY_LOG_WARNING("CreateRayTracingPipeline: Recursion Depth is greater than max ray recursion depth");
            recursuionDepth = mRayTracingProperties.maxRayRecursionDepth;
        }

        //Create pipeline
        auto pipelineInf = vk::RayTracingPipelineCreateInfoKHR()
            .setStageCount(static_cast<uint32_t>(shaderStages.size()))
            .setPStages(shaderStages.data())
            .setGroupCount(static_cast<uint32_t>(shaderGroups.size()))
            .setPGroups(shaderGroups.data())
            .setMaxPipelineRayRecursionDepth(recursuionDepth) 
            .setLayout(layout)
            .setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);

        auto pipeline = mDevice.createRayTracingPipelineKHR(nullptr, nullptr, pipelineInf, nullptr, mDynLoader);

        if(pipeline.result != vk::Result::eSuccess)
        {
            VULRAY_LOG_ERROR("CreateRayTracingPipeline: Failed to create ray tracing pipeline");    
            pipeline.value = nullptr;
        }
        
        return pipeline.value;
        
    }


    void VulrayDevice::DispatchRays(vk::CommandBuffer cmdBuf, const vk::Pipeline rtPipeline, const SBTBuffer& buffer, uint32_t width, uint32_t height, uint32_t depth)
    {
        //dispatch rays
    
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);
        cmdBuf.traceRaysKHR(
            &buffer.RayGenRegion,
            &buffer.MissRegion,
            &buffer.HitGroupRegion,
            &buffer.CallableRegion,
            width, height, depth,
            mDynLoader);

    }



}

static void ErrorCheckShader(const char *shaderName, const vr::Shader &sdr, vk::ShaderStageFlagBits state)
{
    //check is module is valid
    if(!sdr.Module)
    {
        VULRAY_FLOG_ERROR("{0} module is invalid!", shaderName);
        return;
    }
    if(sdr.Stage != state)
    {
        VULRAY_FLOG_ERROR("{0} Stage is not {1}!", shaderName, vk::to_string(state));
    }
  

}
