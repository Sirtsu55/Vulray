#pragma once

namespace vr
{
    /// @brief Enum that defines the type of shader in the shader binding table
    enum class ShaderGroup : uint8_t
    {
        RayGen = 0,
        Miss = 1,
        HitGroup = 2,
        Callable = 3
    };

    /// @brief Contains all shaders that will be used in a hit group of an SBT
    struct HitGroup
    {
        Shader ClosestHitShader     = {};
        Shader AnyHitShader         = {};
        Shader IntersectionShader   = {};
        
    };

    /// @brief Structure that defines a Shader Binding Table that can be used to trace rays
    struct SBTBuffer
    {
        AllocatedBuffer RayGenBuffer    = {};
        AllocatedBuffer MissBuffer      = {};
        AllocatedBuffer HitGroupBuffer  = {};
        AllocatedBuffer CallableBuffer  = {};


        /* 
        Offsets define the start of the shader records in bytes. 
        Next shader record is at offset + ShaderBindingTable::{ShaderType}GroupSize
        */

        vk::StridedDeviceAddressRegionKHR RayGenRegion      = {};
        vk::StridedDeviceAddressRegionKHR MissRegion        = {};
        vk::StridedDeviceAddressRegionKHR HitGroupRegion    = {};
        vk::StridedDeviceAddressRegionKHR CallableRegion    = {};
    };


    /// @brief Pipeline library input structure
    struct RayTracingShaderCollection
    {
        std::vector<Shader> RayGenShaders   = {};
        std::vector<Shader> MissShaders     = {};
        std::vector<HitGroup> HitGroups     = {};
        std::vector<Shader> CallableShaders = {};

        /// @brief Pipeline library that contains all the shaders in the collection
        /// Filled by Vulray when creating the pipeline library.
        /// @note Doesn't need to be destroyed, because it is destroyed when the pipeline it is linked to is destroyed
        vk::Pipeline CollectionPipeline = nullptr;

        /// The order of how the shaders in the pipeline are laid out in the pipeline library.
        /// If there are no shaders of a certain type, the next shader type will be laid out where it would have been.
        /// So if no miss shaders are present, the hit groups will be after the ray gen shaders.
        /// | RayGenShaders    |
        /// | MissShaders      |
        /// | HitGroups        |
        /// | CallableShaders  |
    };

    /// @brief Contains all the shaders that will be used in a shader binding table and the shader record size for each shader type
    /// This structure is used to build the SBTBuffer and the pipeline
    struct ShaderBindingTable
    {
        RayTracingShaderCollection Shaders = {};
        uint32_t RayGenShaderRecordSize     = 0; //size of each ray gen shader record
        uint32_t MissShaderRecordSize       = 0; //size of each miss shader record
        uint32_t HitShaderRecordSize        = 0; //size of each hit group shader record
        uint32_t CallableShaderRecordSize   = 0; //size of each callable shader record
    };

    /// @brief Structure that defines the settings for a ray tracing pipeline. When creating pipeline libraries that 
    ///  link to a single pipeline, the settigns should be the same for all pipelines
    struct PipelineSettings
    {
        vk::PipelineLayout PipelineLayout;

        /// @brief The maximum number of levels of recursion allowed in the pipeline
        uint32_t MaxRecursionDepth = 1;

        /// @brief The maximum size of the payload in bytes
        uint32_t MaxPayloadSize = 0;

        /// @brief The maximum size of the hit attribute in bytes in HitGroups
        uint32_t MaxHitAttributeSize = 0;
    };

    /// @brief Structure that defines the information needed to create a shader binding table
    struct ShaderBindingTableInfo
    {
        /// @brief How many ray gen shaders are in the shader binding table
        uint32_t RayGenShaderCount = 0;
        /// @brief The size of each ray gen shader record in bytes
        uint32_t RayGenShaderRecordSize = 0;

        /// @brief How many miss shaders are in the shader binding table
        uint32_t MissShaderCount = 0;
        /// @brief The size of each miss shader record in bytes
        uint32_t MissShaderRecordSize = 0;

        /// @brief How many hit groups are in the shader binding table
        uint32_t HitGroupCount = 0;
        /// @brief The size of each hit group shader record in bytes
        uint32_t HitGroupRecordSize = 0;

        /// @brief How many callable shaders are in the shader binding table
        uint32_t CallableShaderCount = 0;
        /// @brief The size of each callable shader record in bytes
        uint32_t CallableShaderRecordSize = 0;


        /// Graph of how the shaders might be mixed in a Full Pipeline
        /// The shaders can be mixed in any way, but this is just an example
        /// | Pipeline                      | SBT Index |
        /// | ShaderCollection1.Rgen        | 0         |
        /// | ShaderCollection1.Miss        | 1         |
        /// | ShaderCollection1.HitGroup    | 2         |
        /// | ShaderCollection1.Callable    | 3         |
        /// | ShaderCollection2.Miss        | 4         |
        /// | ShaderCollection2.HitGroup1   | 5         |
        /// | ShaderCollection2.HitGroup2   | 6         |
        /// | ShaderCollection2.Callable    | 7         |
        /// | ShaderCollection3.Rgen        | 8         |
        /// These shaders must be organised per shader type
        /// That would turn the graph into this
        /// | Pipeline                      | SBT Index |
        /// | ShaderCollection1.Rgen        | 0         |
        /// | ShaderCollection3.Rgen        | 1         |
        /// | ShaderCollection1.Miss        | 2         | 
        /// | ShaderCollection2.Miss        | 3         |
        /// | ShaderCollection1.HitGroup    | 4         |
        /// | ShaderCollection2.HitGroup1   | 5         |
        /// | ShaderCollection2.HitGroup2   | 6         |
        /// | ShaderCollection1.Callable    | 7         |
        /// | ShaderCollection2.Callable    | 8         |


        /// @brief These vectors contain where the raygen shaders live in the pipeline
        std::vector<uint32_t> RayGenIndices = {};

        /// @brief These vectors where the miss shaders live in the pipeline
        std::vector<uint32_t> MissIndices = {};

        /// @brief These vectors where the hit group shaders live in the pipeline
        std::vector<uint32_t> HitGroupIndices = {};

        /// @brief These vectors where the callable shaders live in the pipeline
        std::vector<uint32_t> CallableIndices = {};

    };

}