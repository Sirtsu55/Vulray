#include "Vulray/AccelStruct.h"
#include "Vulray/VulrayDevice.h"


namespace vr
{

    BLASHandle VulrayDevice::CreateBLAS(BLASCreateInfo& info)
    {
        auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(info.Flags)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setPGeometries(info.Geometries.data())
            .setGeometryCount(static_cast<uint32_t>(info.Geometries.size()))
            .setPNext(nullptr);
            

        uint32_t maxPrimitiveCount = 0;

        for (auto& geom : info.Geometries)
            maxPrimitiveCount += geom.geometry.triangles.maxVertex;

        BLASHandle outAccel;

        auto buildSizes = vk::AccelerationStructureBuildSizesInfoKHR();

        mDevice.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &buildInfo, &maxPrimitiveCount, &buildSizes, mDynLoader);

        outAccel.pScratchBuffer = new AllocatedBuffer();

        //Create scratch buffer
        *outAccel.pScratchBuffer = CreateBuffer(buildSizes.buildScratchSize,
            VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            vk::BufferUsageFlagBits::eStorageBuffer,
            mAccelProperties.minAccelerationStructureScratchOffsetAlignment);



        //Create buffer for AS
        outAccel.AccelBuf = CreateBuffer(buildSizes.accelerationStructureSize,
            VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);



        auto asInfo = vk::AccelerationStructureCreateInfoKHR()
			.setBuffer(outAccel.AccelBuf.Buffer)
            .setSize(buildSizes.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);


        //only create the AS, don't build it
        outAccel.AccelStruct = mDevice.createAccelerationStructureKHR(asInfo, nullptr, mDynLoader);
        
        auto accelAddInf = vk::AccelerationStructureDeviceAddressInfoKHR()
            .setAccelerationStructure(outAccel.AccelStruct);

        outAccel.AccelBuf.DevAddress = mDevice.getAccelerationStructureAddressKHR(accelAddInf, mDynLoader);
        
        outAccel.GeomBuildInfo = buildInfo;

        return outAccel;
    }

    TLASHandle VulrayDevice::UpdateTLAS(TLASHandle accel, TLASCreateInfo &info)
    {

        auto buildGeometry = vk::AccelerationStructureGeometryDataKHR()
            .setInstances(vk::AccelerationStructureGeometryInstancesDataKHR()
                .setArrayOfPointers(false)
                .setData(accel.InstanceBuffer.DevAddress));

        auto buildGeom = vk::AccelerationStructureGeometryKHR()
            .setGeometry(buildGeometry)
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);

        auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setFlags(info.Flags)
            // build mode, not update, because we are creating a new AS and updating the old one  
            // degrades the TLAS quality, and build time is not that long
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild) 
            .setPGeometries(&buildGeom)
            .setGeometryCount(1);

        auto buildSizes = vk::AccelerationStructureBuildSizesInfoKHR();

        mDevice.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &buildInfo, &info.MaxInstanceCount, &buildSizes, mDynLoader);




        auto asInfo = vk::AccelerationStructureCreateInfoKHR()
            .setBuffer(accel.AccelBuffer.Buffer)
            .setSize(buildSizes.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel);


        accel.AccelStruct = mDevice.createAccelerationStructureKHR(asInfo, nullptr, mDynLoader);
        accel.GeomBuildInfo = buildInfo;

        return accel;
    }

    TLASHandle VulrayDevice::CreateTLAS(TLASCreateInfo &info)
    {
        TLASHandle outAccel;

        outAccel.InstanceBuffer = CreateBuffer(info.MaxInstanceCount * sizeof(vk::AccelerationStructureInstanceKHR),
            VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);

        auto buildGeometry = vk::AccelerationStructureGeometryDataKHR()
            .setInstances(vk::AccelerationStructureGeometryInstancesDataKHR()
                .setArrayOfPointers(false)
                .setData(outAccel.InstanceBuffer.DevAddress));

        outAccel.InstanceGeom = vk::AccelerationStructureGeometryKHR()
            .setGeometry(buildGeometry)
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);

        outAccel.GeomBuildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setFlags(info.Flags)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setPGeometries(&outAccel.InstanceGeom)
            .setGeometryCount(1);

        auto buildSizes = vk::AccelerationStructureBuildSizesInfoKHR();

        mDevice.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &outAccel.GeomBuildInfo, &info.MaxInstanceCount, &buildSizes, mDynLoader);

        outAccel.AccelBuffer = CreateBuffer(buildSizes.accelerationStructureSize,
            0,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

        outAccel.ScratchBuffer = CreateBuffer(buildSizes.buildScratchSize,
            VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            vk::BufferUsageFlagBits::eStorageBuffer,
            mAccelProperties.minAccelerationStructureScratchOffsetAlignment);

        
        auto asInfo = vk::AccelerationStructureCreateInfoKHR()
            .setBuffer(outAccel.AccelBuffer.Buffer)
            .setSize(buildSizes.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel);


        //only create the AS, don't build it
        outAccel.AccelStruct = mDevice.createAccelerationStructureKHR(asInfo, nullptr, mDynLoader);


        return outAccel;
    }

    void VulrayDevice::BuildTLAS(TLASHandle &accel, TLASCreateInfo &info, vk::CommandBuffer cmdBuf)
    {
        auto buildRange = vk::AccelerationStructureBuildRangeInfoKHR()
            .setFirstVertex(0)
            .setTransformOffset(0)
            .setPrimitiveOffset(0)
            .setPrimitiveCount(info.MaxInstanceCount);

        auto pBuildRange = &buildRange;

        accel.GeomBuildInfo.pGeometries = &accel.InstanceGeom;
        accel.GeomBuildInfo.dstAccelerationStructure = accel.AccelStruct;
        accel.GeomBuildInfo.scratchData = accel.ScratchBuffer.DevAddress;
        cmdBuf.buildAccelerationStructuresKHR(1, &accel.GeomBuildInfo, &pBuildRange, mDynLoader);
        //accel build barrier for for next build
        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, 
            (vk::DependencyFlagBits)0,
            0, nullptr, 0, nullptr, 0, nullptr);
    }
    
    void VulrayDevice::UpdateTLASInstances(const TLASHandle& tlas, void* instances, uint32_t instanceCount, uint32_t offset)
    {
        auto maxInstances = tlas.InstanceBuffer.Size / sizeof(vk::AccelerationStructureInstanceKHR);
        if(offset + instanceCount > maxInstances)
        {
            VULRAY_LOG_RED("Instance Update Failed: Instance Count + Offset > TLAS max instances, please make new TLAS with greater maxInstance");
            return;
        }
        
        UpdateBuffer(tlas.InstanceBuffer, instances, sizeof(vk::AccelerationStructureInstanceKHR) * instanceCount, offset * sizeof(vk::AccelerationStructureInstanceKHR));
    }
    
    void VulrayDevice::BuildBLAS(std::vector<BLASHandle> &accels, std::vector<BLASCreateInfo> &infos, vk::CommandBuffer cmdBuf)
    {
        auto buildRange = std::vector<vk::AccelerationStructureBuildRangeInfoKHR>(accels.size()); // the build range info for each BLAS

        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> pBuildRange(accels.size()); // vector containing pointers to the build range info for each BLAS
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildInfos(accels.size());// vector containing the build info for each BLAS
        
        for (int i = 0; i < accels.size(); i++)
        {
            uint32_t maxPrimitiveCount = 0;
            for (auto& geom : infos[i].Geometries)
                maxPrimitiveCount += geom.geometry.triangles.maxVertex;

            buildRange[i] = vk::AccelerationStructureBuildRangeInfoKHR()
                .setFirstVertex(0)
                .setTransformOffset(0)
                .setPrimitiveOffset(0)
                .setPrimitiveCount(maxPrimitiveCount); 

            pBuildRange[i] = &buildRange[i];

            accels[i].GeomBuildInfo.dstAccelerationStructure = accels[i].AccelStruct;
            accels[i].GeomBuildInfo.scratchData = accels[i].pScratchBuffer->DevAddress;

            buildInfos[i] = accels[i].GeomBuildInfo;
        }


        cmdBuf.buildAccelerationStructuresKHR(buildInfos, pBuildRange, mDynLoader);
        //accel build barrier for for next build
        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, 
            (vk::DependencyFlagBits)0,
            0, nullptr, 0, nullptr, 0, nullptr);
        

    }

    void VulrayDevice::BuildBLAS(BLASHandle &accel, BLASCreateInfo &info, vk::CommandBuffer cmdBuf)
    {
        uint32_t maxPrimitiveCount = 0;
        for (auto& geom : info.Geometries)
            maxPrimitiveCount += geom.geometry.triangles.maxVertex;

        auto buildRange = vk::AccelerationStructureBuildRangeInfoKHR()
            .setFirstVertex(0)
            .setTransformOffset(0)
            .setPrimitiveOffset(0)
            .setPrimitiveCount(maxPrimitiveCount);

        auto pBuildRange = &buildRange;

        accel.GeomBuildInfo.dstAccelerationStructure = accel.AccelStruct;
        accel.GeomBuildInfo.scratchData = accel.pScratchBuffer->DevAddress;



        cmdBuf.buildAccelerationStructuresKHR(1, &accel.GeomBuildInfo, &pBuildRange, mDynLoader);
        //accel build barrier for for next build
        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, 
            (vk::DependencyFlagBits)0,
            0, nullptr, 0, nullptr, 0, nullptr);

    }




    void VulrayDevice::PostBuildBLASCleanup(BLASHandle& accel)
    {
        //Destory scratch buffers used to build AS
        vmaDestroyBuffer(mVMAllocator, accel.pScratchBuffer->Buffer, accel.pScratchBuffer->Allocation);

        delete accel.pScratchBuffer;
        accel.pScratchBuffer = nullptr;

    }

    void VulrayDevice::DestroyUpdatedTLAS(TLASHandle &accel)
    {
        //only destroy the accel, other buffers are still needed for updates
        mDevice.destroyAccelerationStructureKHR(accel.AccelStruct, nullptr, mDynLoader);
        //DestroyTLAS() call will destroy all the other buffers
    }
    void VulrayDevice::DestroyBLAS(BLASHandle &accel)
    {
        if(accel.AccelStruct)
            mDevice.destroyAccelerationStructureKHR(accel.AccelStruct, nullptr, mDynLoader);

        if (accel.AccelBuf.Buffer)
            DestroyBuffer(accel.AccelBuf);

        //Destroy scratch buffer, if user forgot to call VulrayDevice::PostBuildBLASCleanup
        if (accel.pScratchBuffer)
        {
            vmaDestroyBuffer(mVMAllocator, accel.pScratchBuffer->Buffer, accel.pScratchBuffer->Allocation);
            delete accel.pScratchBuffer;
            accel.pScratchBuffer = nullptr;
        }
    }

    void VulrayDevice::DestroyTLAS(TLASHandle &accel)
    {
        if(accel.AccelStruct)
            mDevice.destroyAccelerationStructureKHR(accel.AccelStruct, nullptr, mDynLoader);
        if(accel.AccelBuffer.Buffer)
            vmaDestroyBuffer(mVMAllocator, accel.AccelBuffer.Buffer, accel.AccelBuffer.Allocation);
        if(accel.ScratchBuffer.Buffer)
            vmaDestroyBuffer(mVMAllocator, accel.ScratchBuffer.Buffer, accel.ScratchBuffer.Allocation);
        if(accel.InstanceBuffer.Buffer)
            vmaDestroyBuffer(mVMAllocator, accel.InstanceBuffer.Buffer, accel.InstanceBuffer.Allocation);
    }

    vk::AccelerationStructureGeometryKHR FillBottomAccelGeometry(
        const GeometryData &info,
        vk::DeviceAddress vertexDevAddress,
        vk::DeviceAddress indexDevAddress,
        vk::DeviceAddress transformDevAddress)
    {
        auto buildGeometry = vk::AccelerationStructureGeometryDataKHR()
            .setTriangles(
                vk::AccelerationStructureGeometryTrianglesDataKHR()
                .setVertexFormat(info.VertexFormat)
                .setVertexData(vertexDevAddress)
                .setVertexStride(info.VertexStride)
                .setMaxVertex(info.TriangleCount)
                .setIndexType(info.IndexType)
                .setIndexData(indexDevAddress)
                .setTransformData(transformDevAddress)
                
            );
        auto buildGeom = vk::AccelerationStructureGeometryKHR()
            .setGeometry(buildGeometry)
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);

        return buildGeom;
    }
}