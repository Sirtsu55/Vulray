#pragma once
#include "Vulray/Buffer.h"

namespace vr
{
    /// @brief Contains the device address of the geometry data
    struct GeometryDeviceAddress
    {
        GeometryDeviceAddress() = default;
        GeometryDeviceAddress(vk::DeviceAddress vertexOrAABBDevAddress, vk::DeviceAddress indexDevAddress): 
            VertexDevAddress(vertexOrAABBDevAddress), IndexDevAddress(indexDevAddress) {}

        union 
        {
            vk::DeviceAddress VertexDevAddress;
            vk::DeviceAddress AABBDevAddress; 
        };

        /// @brief Device address of the index buffer, only used for triangles
        vk::DeviceAddress IndexDevAddress = {};

        /// @brief Buffer containing the transform for the geometry, if this is null, the geometry will use the identity matrix
        vk::DeviceAddress TransformBuffer = {};
    };


struct GeometryData
    {
        /// @brief Type of geometry, either triangles or AABBs
        vk::GeometryTypeKHR Type = vk::GeometryTypeKHR::eTriangles;

        /// @brief Buffer containing the vertices, only used for triangles
        GeometryDeviceAddress DataAddresses = {};

        /// @brief Format of the index buffer, only used for triangles
        vk::IndexType IndexFormat = vk::IndexType::eUint32;

        /// @brief Format of the vertex buffer, only used for triangles
        vk::Format VertexFormat = vk::Format::eR32G32B32Sfloat;

        /// @brief Stride of each element in the vertex buffer or AABB buffer
        uint32_t Stride = 0; 

        /// @brief Number of primitives in the geometry, such as triangles or AABBs
        uint32_t PrimitiveCount = 0;

        ///@brief Flags for the geometry
        vk::GeometryFlagsKHR Flags = vk::GeometryFlagBitsKHR::eOpaque; 
    };

    //--------------------------------------------------------------------------------------
    // BLAS STRUCTURES
    //--------------------------------------------------------------------------------------

    struct BLASCreateInfo
    {
        /// @brief Geometries to be added to the BLAS
        /// @note All the geometries must be of the same type, either triangles or AABBs
        std::vector<GeometryData> Geometries;

        /// @brief Flags for the acceleration structure
        /// @note The flags must be appropriately set for future use, e.g., compaction, update
        vk::BuildAccelerationStructureFlagsKHR Flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    };

    struct BLASBuildInfo
    {
        /// @brief Contains the build sizes for the acceleration structure
        vk::AccelerationStructureBuildSizesInfoKHR BuildSizes = {};

        /// @brief Contains the build info for the acceleration structure
        vk::AccelerationStructureBuildGeometryInfoKHR BuildGeometryInfo = {};

        /// @brief Geometries that are included in the acceleration structure
        /// @note This is a shared pointer to avoid reallocation when the vector is resized
        std::shared_ptr<vk::AccelerationStructureGeometryKHR[]> Geometries = nullptr;

        uint32_t GeometryCount = 0;

        /// @brief Build ranges info for building the acceleration structure
        std::shared_ptr<vk::AccelerationStructureBuildRangeInfoKHR[]> Ranges = nullptr;

        uint32_t RangesCount = 0;
    };

    struct BLASHandle
    {
        /// @brief Raw handle of the acceleration structure
        vk::AccelerationStructureKHR AccelerationStructure = nullptr;

        /// @brief Buffer containing the acceleration structure
        AllocatedBuffer BLASBuffer = {};
    };

    struct BLASUpdateInfo
    {
        /// @brief Indicates the destination BLAS which is getting updated
        BLASHandle* DestinationBLAS = {};

        /// @brief This is the build info that was given when creating the destination BLAS, which will be reused
        BLASBuildInfo DestinationBuildInfo = {};

        /// @brief If the updated geometries are in different buffers, NewGeometryAddresses will contain the device addresses of the primitives
        /// @note This field can be null if the updated geometries are in the same buffers as the source BLAS
        ///       The size of the vector must be the same as the number of geometries in @c SourceBuildInfo
        ///       Transform buffer must be provided if the source BLAS had a transform buffer, else it must be null
        std::vector<GeometryDeviceAddress> NewGeometryAddresses = {};
    };

    struct CompactionRequest
    {
        /// @brief Query pool that will be used to get the compacted size
        vk::QueryPool CompactionQueryPool = nullptr;

        /// @brief All the BLASes that will be compacted
        std::vector<vk::AccelerationStructureKHR> SourceBLAS = {};
    };




    //--------------------------------------------------------------------------------------
    // TLAS STRUCTUES
    //--------------------------------------------------------------------------------------

    struct TLASCreateInfo
    {
        /// @brief Contains the geometries that will be added to the TLAS
        uint32_t MaxInstanceCount = 0;

        /// @brief Device address of the instance buffer
        vk::DeviceAddress InstanceDevAddress = {};

        /// @brief Flags for the acceleration structure
        vk::BuildAccelerationStructureFlagsKHR Flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    };

    
    struct TLASBuildInfo
    {
        /// @brief Contains the build sizes for the acceleration structure
        vk::AccelerationStructureBuildSizesInfoKHR BuildSizes = {};
        
        /// @brief Contains the build info for the acceleration structure
        vk::AccelerationStructureBuildGeometryInfoKHR BuildGeometryInfo = {};

        /// @brief Geometries that are included in the acceleration structure
        std::shared_ptr<vk::AccelerationStructureGeometryKHR> Geometry = {};

        /// @brief Build ranges info for building the acceleration structure
        vk::AccelerationStructureBuildRangeInfoKHR RangeInfo = {};

        /// @brief Max number of instances that can be added to the TLAS
        uint32_t MaxInstanceCount = 0;

    };

    struct TLASHandle
    {
        /// @brief Raw handle of the acceleration structure
        vk::AccelerationStructureKHR AccelerationStructure = nullptr;

        /// @brief Buffer containing the acceleration structure
        AllocatedBuffer TLASBuffer = {};
    };

 

} 
