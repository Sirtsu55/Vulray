#pragma once
#include "Vulray/Buffer.h"

namespace vr
{
    struct TLASHandle
    {
        
        AllocatedBuffer InstanceBuffer;
        AllocatedBuffer AccelBuffer;
        
        AllocatedBuffer ScratchBuffer;

        vk::AccelerationStructureKHR AccelStruct = nullptr;

        vk::AccelerationStructureGeometryKHR InstanceGeom = {};
        vk::AccelerationStructureBuildGeometryInfoKHR GeomBuildInfo = {};
    };

    // Create info for TLAS 
    // When creaing a TLAS, Vulray will create a buffer for the instances
    // in a HostVisible and if possible Device Local memory
    struct TLASCreateInfo
    {

        vk::BuildAccelerationStructureFlagBitsKHR Flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;

        //Describes the number of predicted instances in the acceleration structure
        //so that the memory can be allocated in advance
        uint32_t MaxInstanceCount = 1;
    };

    struct BLASCreateInfo
    {
        vk::BuildAccelerationStructureFlagBitsKHR Flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        std::vector<vk::AccelerationStructureGeometryKHR> Geometries;
        AllocatedBuffer VertexBuffer;
        AllocatedBuffer IndexBuffer;
    };

    struct GeometryData
    {
        uint32_t VertexStride = sizeof(float) * 3;
        vk::Format VertexFormat = vk::Format::eR32G32B32Sfloat;

        void* pIndices = nullptr;
        vk::IndexType IndexType = vk::IndexType::eUint32;

        uint32_t TriangleCount = 0;
    };


    struct BLASHandle
    {
        AllocatedBuffer* pScratchBuffer; // gets allocated on heap when the accel is created and released when PostBuildCleanup is called
        
		AllocatedBuffer AccelBuf;
        
        vk::AccelerationStructureKHR AccelStruct;
        
        vk::AccelerationStructureBuildGeometryInfoKHR GeomBuildInfo;
    };


} 
