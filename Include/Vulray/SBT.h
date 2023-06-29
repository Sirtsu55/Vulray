#pragma once

namespace vr
{
    /// @brief Enum that defines the type of shader in the shader binding table
    enum class ShaderGroup : uint8_t
    {
        RayGen      = 0,
        Miss        = 1,
        HitGroup    = 2,
        Callable    = 3
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

    /// @brief Structure that defines the settings for a ray tracing pipeline. When creating pipeline libraries that 
    ///  link to a single pipeline, the settigns should be the same for all pipelines
    struct PipelineSettings
    {
        vk::PipelineLayout PipelineLayout   = nullptr;

        /// @brief The maximum number of levels of recursion allowed in the pipeline
        uint32_t MaxRecursionDepth          = 1;

        /// @brief The maximum size of the payload in bytes
        uint32_t MaxPayloadSize             = 0;

        /// @brief The maximum size of the hit attribute in bytes in HitGroups
        uint32_t MaxHitAttributeSize        = 0;
    };

    /// @brief Structure that defines the information needed to create a shader binding table
    struct SBTInfo
    {
        /// @brief The size of each ray gen shader record in bytes
        uint32_t RayGenShaderRecordSize     = 0;

        /// @brief The size of each miss shader record in bytes
        uint32_t MissShaderRecordSize       = 0;

        /// @brief The size of each hit group shader record in bytes
        uint32_t HitGroupRecordSize         = 0;

        /// @brief The size of each callable shader record in bytes
        uint32_t CallableShaderRecordSize   = 0;

        /// If expecting more shaders than the pipeline has, reserve space for them
        /// The SBT for each buffer will contain space for (***Indices.size() + Reserve***)

        uint32_t ReserveRayGenGroups    = 0;
        uint32_t ReserveMissGroups      = 0;
        uint32_t ReserveHitGroups       = 0;
        uint32_t ReserveCallableGroups  = 0;

        /// Graph of how the shaders might be mixed in a full pipeline.
        /// The shaders can be mixed in any way, but this is just an example

        /// | Pipeline                      | SBT Index |
        /// ---------------------------------------------
        /// | ShaderCollection1.Rgen        | 0         |
        /// | ShaderCollection1.Miss        | 1         |
        /// | ShaderCollection1.HitGroup    | 2         |
        /// | ShaderCollection1.Callable    | 3         |
        /// | ShaderCollection2.Miss        | 4         |
        /// | ShaderCollection2.HitGroup1   | 5         |
        /// | ShaderCollection2.HitGroup2   | 6         |
        /// | ShaderCollection2.Callable    | 7         |
        /// | ShaderCollection3.Rgen        | 8         |
        /// ---------------------------------------------
        /// We want to support linking new shader collections to the already existing pipeline for efficiency.
        /// This means that the shaders can be mixed in any way and organising them is necessary for the SBT to work.
        /// We need indices for shader types to tell where in the pipeline the shaders live.
        /// Then the shaders go into the SBT organised by their index.
        /// That would turn the graph into this.
        /// | Pipeline                      | SBT Index |
        /// ---------------------------------------------
        /// | ShaderCollection1.Rgen        | 0         |
        /// | ShaderCollection3.Rgen        | 1         |
        /// | ShaderCollection1.Miss        | 2         | 
        /// | ShaderCollection2.Miss        | 3         |
        /// | ShaderCollection1.HitGroup    | 4         |
        /// | ShaderCollection2.HitGroup1   | 5         |
        /// | ShaderCollection2.HitGroup2   | 6         |
        /// | ShaderCollection1.Callable    | 7         |
        /// | ShaderCollection2.Callable    | 8         |
        /// ---------------------------------------------



        /// @brief These vectors contain where the raygen shaders live in the pipeline
        std::vector<uint32_t> RayGenIndices     = {};

        /// @brief These vectors where the miss shaders live in the pipeline
        std::vector<uint32_t> MissIndices       = {};

        /// @brief These vectors where the hit group shaders live in the pipeline
        std::vector<uint32_t> HitGroupIndices   = {};

        /// @brief These vectors where the callable shaders live in the pipeline
        std::vector<uint32_t> CallableIndices   = {};
    };

}