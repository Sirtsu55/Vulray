#include "Vulray/Shader.h"
#include "Vulray/VulrayDevice.h"

namespace vr
{

    std::pair<std::vector<vk::PipelineShaderStageCreateInfo>, std::vector<vk::RayTracingShaderGroupCreateInfoKHR>>
    VulrayDevice::GetShaderStagesAndRayTracingGroups(const RayTracingShaderCollection& info)
    {
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
        shaderStages.reserve(1 + info.MissShaders.size() + info.HitGroups.size() + info.CallableShaders.size());
        shaderGroups.reserve(1 + info.MissShaders.size() + info.HitGroups.size() + info.CallableShaders.size());

        // create ray gen shader groups
        for (auto& shader : info.RayGenShaders)
        {
            shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                                       .setStage(vk::ShaderStageFlagBits::eRaygenKHR)
                                       .setModule(shader.Module)
                                       .setPName(shader.EntryPoint));

            uint32_t rayGenIndex = static_cast<uint32_t>(shaderStages.size() - 1);

            shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
                                       .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                                       .setGeneralShader(rayGenIndex)
                                       .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                                       .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                                       .setIntersectionShader(VK_SHADER_UNUSED_KHR));
        }
        // create miss shader groups
        for (auto& shader : info.MissShaders)
        {
            shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                                       .setStage(vk::ShaderStageFlagBits::eMissKHR)
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
        // create hit group shader groups
        for (auto& hg : info.HitGroups)
        {
            // null init hit group, will be filled in later
            auto hitGroup = vk::RayTracingShaderGroupCreateInfoKHR()
                                // assume triangles for now, change if hit group has a custom intersection shader
                                .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
                                .setGeneralShader(VK_SHADER_UNUSED_KHR)
                                .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                                .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                                .setIntersectionShader(VK_SHADER_UNUSED_KHR);

            // check if both shaders are null
            if (!hg.ClosestHitShader.Module && !hg.AnyHitShader.Module && !hg.IntersectionShader.Module)
            {
                VULRAY_LOG_ERROR("CreateRayTracingPipeline: Hit group must have at least one shader");
            }

            // add closest hit shader if it exists
            if (hg.ClosestHitShader.Module)
            {

                shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                                           .setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
                                           .setModule(hg.ClosestHitShader.Module)
                                           .setPName(hg.ClosestHitShader.EntryPoint));

                uint32_t closestHitIndex = static_cast<uint32_t>(shaderStages.size() - 1);
                hitGroup.setClosestHitShader(closestHitIndex);
            }
            // add any hit shader if it exists
            if (hg.AnyHitShader.Module)
            {
                shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                                           .setStage(vk::ShaderStageFlagBits::eAnyHitKHR)
                                           .setModule(hg.AnyHitShader.Module)
                                           .setPName(hg.AnyHitShader.EntryPoint));

                uint32_t anyHitIndex = static_cast<uint32_t>(shaderStages.size() - 1);
                hitGroup.setAnyHitShader(anyHitIndex);
            }
            // add intersection shader if it exists
            if (hg.IntersectionShader.Module)
            {
                shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                                           .setStage(vk::ShaderStageFlagBits::eIntersectionKHR)
                                           .setModule(hg.IntersectionShader.Module)
                                           .setPName(hg.IntersectionShader.EntryPoint));

                uint32_t intersectionIndex = static_cast<uint32_t>(shaderStages.size() - 1);
                hitGroup.setType(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup);
                hitGroup.setIntersectionShader(intersectionIndex);
            }

            shaderGroups.push_back(hitGroup);
        }
        // create callable shader groups
        for (auto& shader : info.CallableShaders)
        {
            shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                                       .setStage(vk::ShaderStageFlagBits::eCallableKHR)
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
        return std::make_pair(std::move(shaderStages), std::move(shaderGroups));
    }

    std::pair<vk::Pipeline, SBTInfo> VulrayDevice::CreateRayTracingPipeline(
        const RayTracingShaderCollection& shaderCollection, PipelineSettings& settings, vk::PipelineCreateFlags flags,
        vk::DeferredOperationKHR deferredOp)
    {
        vr::SBTInfo sbtInfo = {};

        uint32_t pipelineIndex = 0; // index of shader in the compiled pipeline

        for (auto& shader : shaderCollection.RayGenShaders) sbtInfo.RayGenIndices.push_back(pipelineIndex++);
        for (auto& shader : shaderCollection.MissShaders) sbtInfo.MissIndices.push_back(pipelineIndex++);
        for (auto& shader : shaderCollection.HitGroups) sbtInfo.HitGroupIndices.push_back(pipelineIndex++);
        for (auto& shader : shaderCollection.CallableShaders) sbtInfo.CallableIndices.push_back(pipelineIndex++);

        auto [shaderStages, shderGroups] = GetShaderStagesAndRayTracingGroups(shaderCollection);

        vk::RayTracingPipelineInterfaceCreateInfoKHR interfaceInfo =
            vk::RayTracingPipelineInterfaceCreateInfoKHR()
                .setMaxPipelineRayHitAttributeSize(settings.MaxHitAttributeSize)
                .setMaxPipelineRayPayloadSize(settings.MaxPayloadSize);

        auto pipelineInfo = vk::RayTracingPipelineCreateInfoKHR()
                                .setFlags(flags)
                                .setMaxPipelineRayRecursionDepth(settings.MaxRecursionDepth)
                                .setPLibraryInterface(&interfaceInfo)
                                .setLayout(settings.PipelineLayout)
                                .setGroups(shderGroups)
                                .setStages(shaderStages);

        auto res = mDevice.createRayTracingPipelineKHR(deferredOp, nullptr, pipelineInfo, nullptr, mDynLoader);

        // when deferredOp is not null, the pipeline is created asynchronously, so it doesn't return success or failure
        if (res.result != vk::Result::eSuccess && res.result != vk::Result::eOperationDeferredKHR)
        {
            VULRAY_LOG_ERROR("CreateRayTracingPipeline: Failed to create ray tracing pipeline");
            res.value = nullptr;
        }

        return std::make_pair(res.value, sbtInfo);
    }
    std::pair<vk::Pipeline, SBTInfo> VulrayDevice::CreateRayTracingPipeline(
        const std::vector<RayTracingShaderCollection>& shaderCollections, PipelineSettings& settings,
        vk::PipelineCreateFlags flags, vk::PipelineCache cache, vk::DeferredOperationKHR deferredOp)
    {
        vr::SBTInfo sbtInfo = {};

        std::vector<vk::Pipeline> libPipelines = {};
        libPipelines.reserve(shaderCollections.size());

        uint32_t pipelineIndex = 0; // index of shader in the compiled pipeline

        for (auto& collection : shaderCollections)
        {
            libPipelines.push_back(collection.CollectionPipeline);
            // Assign where in the pipeline the shaders are, for future SBT creation,
            // so opaque handles can be queried for the shader groups
            for (auto& shader : collection.RayGenShaders) sbtInfo.RayGenIndices.push_back(pipelineIndex++);
            for (auto& shader : collection.MissShaders) sbtInfo.MissIndices.push_back(pipelineIndex++);
            for (auto& shader : collection.HitGroups) sbtInfo.HitGroupIndices.push_back(pipelineIndex++);
            for (auto& shader : collection.CallableShaders) sbtInfo.CallableIndices.push_back(pipelineIndex++);
        }

        vk::RayTracingPipelineInterfaceCreateInfoKHR interfaceInfo =
            vk::RayTracingPipelineInterfaceCreateInfoKHR()
                .setMaxPipelineRayHitAttributeSize(settings.MaxHitAttributeSize)
                .setMaxPipelineRayPayloadSize(settings.MaxPayloadSize);

        vk::PipelineLibraryCreateInfoKHR libraryInfo =
            vk::PipelineLibraryCreateInfoKHR().setPLibraries(libPipelines.data()).setLibraryCount(libPipelines.size());

        auto pipelineInfo = vk::RayTracingPipelineCreateInfoKHR()
                                .setFlags(flags)
                                .setMaxPipelineRayRecursionDepth(settings.MaxRecursionDepth)
                                .setPLibraryInterface(&interfaceInfo)
                                .setPLibraryInfo(&libraryInfo)
                                .setLayout(settings.PipelineLayout);

        auto res = mDevice.createRayTracingPipelineKHR(deferredOp, cache, pipelineInfo, nullptr, mDynLoader);
        // when deferredOp is not null, the pipeline is created asynchronously, so it doesn't return success or failure
        if (res.result != vk::Result::eSuccess && res.result != vk::Result::eOperationDeferredKHR)
        {
            VULRAY_LOG_ERROR("CreateRayTracingPipeline: Failed to create ray tracing pipeline");
            res.value = nullptr;
        }

        return std::make_pair(res.value, sbtInfo);
    }

    std::pair<vk::Pipeline, SBTInfo> VulrayDevice::CreateRayTracingPipeline(
        const std::vector<RayTracingShaderCollection>& shaderCollections, PipelineSettings& settings,
        SBTInfo& sbtInfoOld, vk::PipelineCreateFlags flags, vk::PipelineCache cache,
        vk::DeferredOperationKHR deferredOp)
    {
        std::pair<vk::Pipeline, SBTInfo> pipelineInfo =
            CreateRayTracingPipeline(shaderCollections, settings, flags, cache, deferredOp);
        pipelineInfo.second.RayGenShaderRecordSize = sbtInfoOld.RayGenShaderRecordSize;
        pipelineInfo.second.MissShaderRecordSize = sbtInfoOld.MissShaderRecordSize;
        pipelineInfo.second.HitGroupRecordSize = sbtInfoOld.HitGroupRecordSize;
        pipelineInfo.second.CallableShaderRecordSize = sbtInfoOld.CallableShaderRecordSize;

        return pipelineInfo;
    }

    void VulrayDevice::CreatePipelineLibrary(RayTracingShaderCollection& shaderCollection, PipelineSettings& settings,
                                             vk::PipelineCreateFlags flags, vk::PipelineCache cache,
                                             vk::DeferredOperationKHR deferredOp)
    {
        vk::RayTracingPipelineInterfaceCreateInfoKHR interfaceInfo =
            vk::RayTracingPipelineInterfaceCreateInfoKHR()
                .setMaxPipelineRayHitAttributeSize(settings.MaxHitAttributeSize)
                .setMaxPipelineRayPayloadSize(settings.MaxPayloadSize);

        auto [shaderStages, shderGroups] = GetShaderStagesAndRayTracingGroups(shaderCollection);

        auto pipelineInfo = vk::RayTracingPipelineCreateInfoKHR()
                                .setFlags(flags | vk::PipelineCreateFlagBits::eLibraryKHR)
                                .setMaxPipelineRayRecursionDepth(settings.MaxRecursionDepth)
                                .setPLibraryInterface(&interfaceInfo)
                                .setLayout(settings.PipelineLayout)
                                .setGroups(shderGroups)
                                .setStages(shaderStages);

        auto res = mDevice.createRayTracingPipelineKHR(deferredOp, cache, pipelineInfo, nullptr, mDynLoader);

        // when deferredOp is not null, the pipeline is created asynchronously, so it doesn't return success or failure
        if (res.result != vk::Result::eSuccess && res.result != vk::Result::eOperationDeferredKHR)
        {
            VULRAY_LOG_ERROR("CreateRayTracingPipeline: Failed to create ray tracing pipeline");
            res.value = nullptr;
        }

        shaderCollection.CollectionPipeline = res.value;
    }

    void VulrayDevice::DispatchRays(vk::CommandBuffer cmdBuf, const vk::Pipeline rtPipeline, const SBTBuffer& buffer,
                                    uint32_t width, uint32_t height, uint32_t depth)
    {
        // dispatch rays
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);
        cmdBuf.traceRaysKHR(&buffer.RayGenRegion, &buffer.MissRegion, &buffer.HitGroupRegion, &buffer.CallableRegion,
                            width, height, depth, mDynLoader);
    }
} // namespace vr
