#pragma once

namespace vr
{
    struct HitGroup
    {
        Shader ClosestHitShader;
        Shader AnyHitShader;
        Shader IntersectionShader;
        
    };

    struct SBTBuffer
    {
        AllocatedBuffer RayGenBuffer;
        AllocatedBuffer MissBuffer;
        AllocatedBuffer HitGroupBuffer;
        AllocatedBuffer CallableBuffer;


        /* 
        Offsets define the start of the shader records in bytes. 
        Next shader record is at offset + ShaderBindingTable::{ShaderType}GroupSize
        */
        vk::StridedDeviceAddressRegionKHR RayGenRegion = {};
        vk::StridedDeviceAddressRegionKHR MissRegion = {};
        vk::StridedDeviceAddressRegionKHR HitGroupRegion = {};
        vk::StridedDeviceAddressRegionKHR CallableRegion = {};
    };

    struct ShaderBindingTable
    {
        Shader RayGenShader;
        uint32_t RayGenShaderRecordSize = 0; //size of each ray gen shader record
        std::vector<Shader> MissShaders;
        uint32_t MissShaderRecordSize = 0; //size of each miss shader record
        std::vector<HitGroup> HitGroups;
        uint32_t HitShaderRecordSize = 0; //size of each hit group shader record
        std::vector<Shader> CallableShaders;
        uint32_t CallableShaderRecordSize = 0; //size of each callable shader record
    };

    enum class ShaderGroup : uint8_t
    {
        RayGen = 0,
        Miss = 1,
        HitGroup = 2,
        Callable = 3
    };
}