#pragma once
#include "Vulray/Buffer.h"

namespace vr
{
    // Contains the device address of the geometry data
    // so either the (vertex buffer and index buffer) or the (AABB buffer)
    struct GeometryDeviceAddress
    {
        GeometryDeviceAddress() = default;
        GeometryDeviceAddress(vk::DeviceAddress vertexOrAABBDevAddress, vk::DeviceAddress indexDevAddress): 
            // when vertexDevAddress is updated, AABBDevAddress is updated as well, because they are a union
            // therefore we don't need to update AABBDevAddress here, as it is also updated
            VertexDevAddress(vertexOrAABBDevAddress), IndexDevAddress(indexDevAddress) {}

        union 
        {
            vk::DeviceAddress VertexDevAddress; //For triangles
            vk::DeviceAddress AABBDevAddress; //For AABBs
        };

        vk::DeviceAddress IndexDevAddress = {}; //For triangles, if building AABBs, this should be null, and will be ignored

        //Buffer containing the transform for the geometry, if this is null, the geometry will use the identity matrix
        vk::DeviceAddress TransformBuffer = {};
    };


    struct GeometryData
    {
        // IMPORTANT NOTE
        // the offsets/starts in vk::AccelerationStructureBuildRangeInfoKHR will be 0
        // that means the user is responsible for settings device addresses with the offsets
        // if they have multiple geometries in the same buffer
        vk::GeometryTypeKHR Type = vk::GeometryTypeKHR::eTriangles; //Type of geometry, either triangles or AABBs

        GeometryDeviceAddress DataAddresses = {}; //Buffer containing the vertices, only used for triangles

        vk::IndexType IndexFormat = vk::IndexType::eUint32; //Format of the index buffer, only used for triangles
        vk::Format VertexFormat = vk::Format::eR32G32B32Sfloat; //Format of the vertex buffer, only used for triangles

        uint32_t Stride = 0; //Stride of each element in the vertex buffer or AABB buffer

        //Number of primitives in the geometry, so for triangles, this is the number of triangles and for AABBs, this is the number of AABBs
        uint32_t PrimitiveCount = 0; 

        //Flags for the geometry
        vk::GeometryFlagsKHR Flags = vk::GeometryFlagBitsKHR::eOpaque;

    };
    
    //--------------------------------------------------------------------------------------
    // BLAS STRUCTUES
    //--------------------------------------------------------------------------------------
    
    struct BLASCreateInfo
    {
        // all geometries must be the same type either triangles or AABBs
        std::vector<GeometryData> Geometries;
        vk::BuildAccelerationStructureFlagsKHR Flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    };

    //Contains all the information needed to build the acceleration structure
    struct BLASBuildInfo
    {
        vk::AccelerationStructureBuildSizesInfoKHR BuildSizes = {};
        vk::AccelerationStructureBuildGeometryInfoKHR BuildGeometryInfo = {};

        //to avoid std::vector reallocation, we store the geometries and ranges in a smart pointer
        std::shared_ptr<vk::AccelerationStructureGeometryKHR[]> Geometries = nullptr;
        uint32_t GeometryCount = 0;

        std::shared_ptr<vk::AccelerationStructureBuildRangeInfoKHR[]> Ranges = nullptr;
        uint32_t RangesCount = 0;
    };

    struct BLASHandle
    {
        vk::AccelerationStructureKHR AccelerationStructure = nullptr;
        AllocatedBuffer BLASBuffer = {};
    };

    struct BLASUpdateInfo
    {
        // A pointer so that we don't replicate the BLASHandle and cause unnecessary copies
        BLASHandle* SourceBLAS = {};

        // The build info for will be reused from the source BLAS
        BLASBuildInfo SourceBuildInfo = {};

        // contains the device addresses of the updated geometry
        // usually this will be the same as the source BLAS, but the triangle/aabb data will be different
        // transform buffer must be provided if the source BLAS had a transform buffer, else it must be null
        std::vector<GeometryDeviceAddress> NewGeometryAddresses = {};

    };

    //--------------------------------------------------------------------------------------
    // TLAS STRUCTUES
    //--------------------------------------------------------------------------------------

    struct TLASCreateInfo
    {
        uint32_t MaxInstanceCount = 0;
        vk::DeviceAddress InstanceDevAddress = {};
        vk::BuildAccelerationStructureFlagsKHR Flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    };

    struct TLASBuildInfo
    {
        //TLAS can only be created with a single geometry

        vk::AccelerationStructureBuildSizesInfoKHR BuildSizes = {};
        
        vk::AccelerationStructureBuildGeometryInfoKHR BuildGeometryInfo = {};

        std::shared_ptr<vk::AccelerationStructureGeometryKHR> Geometry = {};

        vk::AccelerationStructureBuildRangeInfoKHR RangeInfo = {};

        uint32_t MaxInstanceCount = 0;

    };

    struct TLASHandle
    {
        AllocatedBuffer TLASBuffer = {};

        AllocatedBuffer InstancesBuffer = {}; // buffer containing the instances for the TLAS
        
        AllocatedBuffer ScratchBuffer = {}; // scratch buffer for building the TLAS will be reused when updating the TLAS

        vk::AccelerationStructureKHR AccelerationStructure = nullptr;

    };

} 
