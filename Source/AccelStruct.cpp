#include "Vulray/AccelStruct.h"
#include "Vulray/VulrayDevice.h"


namespace vr
{

    //--------------------------------------------------------------------------------------
    // BLAS FUCNTIONS
    //--------------------------------------------------------------------------------------

    std::pair<BLASHandle, BLASBuildInfo> VulrayDevice::CreateBLAS(const BLASCreateInfo& info)
    {
        BLASBuildInfo outBuildInfo = {};
        BLASHandle outAccel = {};
        uint32_t geomSize = info.Geometries.size();

        std::vector<uint32_t> maxPrimitiveCounts(geomSize);

        // create a unique pointer to the array of geometries
        outBuildInfo.Geometries = std::make_shared<vk::AccelerationStructureGeometryKHR[]>(geomSize); 
        outBuildInfo.GeometryCount = geomSize;

        outBuildInfo.Ranges = std::make_shared<vk::AccelerationStructureBuildRangeInfoKHR[]>(geomSize);
        outBuildInfo.RangesCount = geomSize; 

        for (size_t i = 0; i < geomSize; i++)
        {
            //Convert the geometries to the vulkan format
            outBuildInfo.Geometries[i] = vk::AccelerationStructureGeometryKHR()
                .setGeometry(ConvertToVulkanGeometry(info.Geometries[i]))
                .setFlags(info.Geometries[i].Flags)
                .setGeometryType(info.Geometries[i].Type);

            // Fill in the range info
            outBuildInfo.Ranges[i] = vk::AccelerationStructureBuildRangeInfoKHR()
				.setFirstVertex(0)
				.setPrimitiveCount(info.Geometries[i].PrimitiveCount)
				.setPrimitiveOffset(0)
				.setTransformOffset(0);

            maxPrimitiveCounts[i] = info.Geometries[i].PrimitiveCount;


        }

        //Create the build info
        outBuildInfo.BuildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(info.Flags)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setDstAccelerationStructure(nullptr)
            .setPGeometries(outBuildInfo.Geometries.get())
            .setGeometryCount(outBuildInfo.GeometryCount);


        // Get the size requirements for the acceleration structure
        mDevice.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            &outBuildInfo.BuildGeometryInfo,
            maxPrimitiveCounts.data(),
            &outBuildInfo.BuildSizes,
            mDynLoader); // This will fill in the size requirements

        //Create the buffer for the acceleration structure
        outAccel.BLASBuffer = CreateBuffer(outBuildInfo.BuildSizes.accelerationStructureSize, 
            0, // no flags for VMA
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);
        

        //Create the acceleration structure
        auto createInfo = vk::AccelerationStructureCreateInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setBuffer(outAccel.BLASBuffer.Buffer)
            .setSize(outBuildInfo.BuildSizes.accelerationStructureSize);
        
        outAccel.AccelerationStructure = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

        //Get the address of the acceleration structure, because it may vary from getBufferDeviceAddress(...)
        outAccel.BLASBuffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR()
            .setAccelerationStructure(outAccel.AccelerationStructure), mDynLoader);
            
        // Fill in the build info with the acceleration structure
        outBuildInfo.BuildGeometryInfo.setDstAccelerationStructure(outAccel.AccelerationStructure);

        return std::make_pair(outAccel, outBuildInfo);
    }


    void VulrayDevice::BuildBLAS(const std::vector<BLASBuildInfo> &buildInfos, vk::CommandBuffer cmdBuf)
    {
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos;
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
        pBuildRangeInfos.reserve(buildInfos.size());
        buildGeometryInfos.reserve(buildInfos.size());
        uint32_t scratchSize = 0;
        // loop over build infos and get scratch sizes
        for(uint32_t i = 0; i < buildInfos.size(); i++)
        {
            pBuildRangeInfos.push_back(buildInfos[i].Ranges.get());
            buildGeometryInfos.push_back(buildInfos[i].BuildGeometryInfo);
        }

        //Build the acceleration structures
        cmdBuf.buildAccelerationStructuresKHR(buildInfos.size(), buildGeometryInfos.data(), pBuildRangeInfos.data(), mDynLoader);
        

    }

    BLASBuildInfo VulrayDevice::UpdateBLAS(BLASUpdateInfo& updateInfo)
    {

        bool useSourceDeviceAddress = updateInfo.NewGeometryAddresses.size() == 0;
        
        uint32_t geomSize = updateInfo.SourceBuildInfo.GeometryCount;

        BLASBuildInfo outBuildInfo = updateInfo.SourceBuildInfo;

        outBuildInfo.BuildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eUpdate);

        std::vector<uint32_t> maxPrimitiveCounts(geomSize);

        // setup the primitive counts and ranges
        for (uint32_t i = 0; i < geomSize; i++)
        {
            if (!useSourceDeviceAddress)
            {
                auto geomType = updateInfo.SourceBuildInfo.Geometries[i].geometryType;
                if(geomType == vk::GeometryTypeKHR::eTriangles)
                {
                    outBuildInfo.Geometries[i].geometry.triangles.vertexData = updateInfo.NewGeometryAddresses[i].VertexDevAddress;
                    outBuildInfo.Geometries[i].geometry.triangles.indexData = updateInfo.NewGeometryAddresses[i].IndexDevAddress;
                }
                else if(geomType == vk::GeometryTypeKHR::eAabbs)
                {
                    outBuildInfo.Geometries[i].geometry.aabbs.data = updateInfo.NewGeometryAddresses[i].VertexDevAddress;
                }
            }
            

            maxPrimitiveCounts[i] = updateInfo.SourceBuildInfo.Ranges[i].primitiveCount;
            outBuildInfo.Ranges[i].primitiveOffset = 0;
            outBuildInfo.Ranges[i].primitiveCount = updateInfo.SourceBuildInfo.Ranges[i].primitiveCount;
        }
        

        // Get the size requirements for the acceleration structure
        mDevice.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            &outBuildInfo.BuildGeometryInfo,
            maxPrimitiveCounts.data(),
            &outBuildInfo.BuildSizes,
            mDynLoader); // This will fill in the size requirements


        // seup dst and src acceleration structures
        outBuildInfo.BuildGeometryInfo.srcAccelerationStructure = updateInfo.SourceBLAS->AccelerationStructure;
        outBuildInfo.BuildGeometryInfo.dstAccelerationStructure = updateInfo.SourceBLAS->AccelerationStructure;
        return outBuildInfo;
    }

    AllocatedBuffer VulrayDevice::CreateScratchBufferBLAS(std::vector<BLASBuildInfo> &buildInfos)
    {
        uint32_t scratchSize = 0;
        // loop over build infos and get scratch sizes
        for (auto& info : buildInfos)
        {
            scratchSize += AlignUp(info.BuildSizes.buildScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
        }

        auto outScratchBuffer = CreateBuffer(scratchSize, 0,
            vk::BufferUsageFlagBits::eStorageBuffer,
            mAccelProperties.minAccelerationStructureScratchOffsetAlignment);

        // loop over build infos and set scratch buffers
        // this will offset the scratch buffer so that each build info has its own region of the scratch buffer
        vk::DeviceAddress scratchDataAddr = outScratchBuffer.DevAddress;

        for (auto& info : buildInfos)
        {
			info.BuildGeometryInfo.setScratchData(scratchDataAddr);
            scratchDataAddr += AlignUp(info.BuildSizes.buildScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
		}
        return outScratchBuffer;
    }

    AllocatedBuffer VulrayDevice::CreateScratchBufferBLAS(BLASBuildInfo &buildInfo)
    {
        uint32_t scratchSize = AlignUp(buildInfo.BuildSizes.buildScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);

        auto outScratchBuffer = CreateBuffer(scratchSize, 0,
            vk::BufferUsageFlagBits::eStorageBuffer,
            mAccelProperties.minAccelerationStructureScratchOffsetAlignment);

        // loop over build infos and set scratch buffers
        // this will offset the scratch buffer so that each build info has its own region of the scratch buffer
        vk::DeviceAddress scratchDataAddr = outScratchBuffer.DevAddress;

        buildInfo.BuildGeometryInfo.setScratchData(scratchDataAddr);
        return outScratchBuffer;
    }
    //--------------------------------------------------------------------------------------
    // TLAS FUCNTIONS
    //--------------------------------------------------------------------------------------

    std::pair<TLASHandle, TLASBuildInfo> VulrayDevice::CreateTLAS(const TLASCreateInfo& info)
    {
        TLASHandle outAccel = {};
        TLASBuildInfo outBuildInfo = {};

        auto buildGeometry = vk::AccelerationStructureGeometryDataKHR()
            .setInstances(vk::AccelerationStructureGeometryInstancesDataKHR()
                .setArrayOfPointers(false));

        outBuildInfo.Geometry = std::make_shared<vk::AccelerationStructureGeometryKHR>(vk::AccelerationStructureGeometryKHR()
            .setGeometry(buildGeometry)
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque));
            
        //Create the build info
        outBuildInfo.BuildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setFlags(info.Flags)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setDstAccelerationStructure(nullptr)
            .setPGeometries(outBuildInfo.Geometry.get())
            .setGeometryCount(1);

        // Fill Range Info
        outBuildInfo.RangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
            .setPrimitiveCount(0) // number of instances, will be modified in the build function
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);

        // Get the size requirements for the acceleration structure
        mDevice.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            &outBuildInfo.BuildGeometryInfo,
            &info.MaxInstanceCount, // max number of instances/primitives in the geometry, which is always 1 for TLAS builds
            &outBuildInfo.BuildSizes,
            mDynLoader); 
        
        //Create the buffer for the acceleration structure
        outAccel.TLASBuffer = CreateBuffer(outBuildInfo.BuildSizes.accelerationStructureSize, 
            0, // no flags for VMA
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);
        

        //Create the acceleration structure
        auto createInfo = vk::AccelerationStructureCreateInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setBuffer(outAccel.TLASBuffer.Buffer)
            .setSize(outBuildInfo.BuildSizes.accelerationStructureSize);
        
        outAccel.AccelerationStructure = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

        outAccel.TLASBuffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR()
            .setAccelerationStructure(outAccel.AccelerationStructure), mDynLoader);

        // Fill in the build info with the acceleration structure

        outBuildInfo.BuildGeometryInfo.setDstAccelerationStructure(outAccel.AccelerationStructure);

        outBuildInfo.MaxInstanceCount = info.MaxInstanceCount;

        return std::make_pair(outAccel, outBuildInfo);
    }

    void VulrayDevice::BuildTLAS(TLASBuildInfo& buildInfo, 
            const AllocatedBuffer& InstanceBuffer, uint32_t instanceCount, 
            vk::CommandBuffer cmdBuf)
    {

        buildInfo.RangeInfo.primitiveCount = instanceCount;

        buildInfo.Geometry->geometry.instances.data = InstanceBuffer.DevAddress;

        auto* pBuildRangeInfo = &buildInfo.RangeInfo;

        //Build the acceleration structure
        cmdBuf.buildAccelerationStructuresKHR(1, &buildInfo.BuildGeometryInfo, &pBuildRangeInfo, mDynLoader);

    }

    std::pair<TLASHandle, TLASBuildInfo> VulrayDevice::UpdateTLAS(TLASHandle& oldTLAS, TLASBuildInfo& oldBuildInfo, bool destroyOld)
    {
        TLASHandle outAccel = oldTLAS;
        TLASBuildInfo outBuildInfo = oldBuildInfo;

        // Create new acceleration structure
        // sizes are the same as the old one, because we are only creating a new one with the same number of instances
        // otherwise we would need create a new one with the new number of instances, from CreateTLAS(...)

        auto createInfo = vk::AccelerationStructureCreateInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setBuffer(outAccel.TLASBuffer.Buffer)
            .setSize(outBuildInfo.BuildSizes.accelerationStructureSize);

        outAccel.AccelerationStructure = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

        outAccel.TLASBuffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR()
            .setAccelerationStructure(outAccel.AccelerationStructure), mDynLoader);

        outBuildInfo.BuildGeometryInfo.setDstAccelerationStructure(outAccel.AccelerationStructure);

        // destroy the old one
        if(destroyOld)
            DestroyAccelerationStructure(oldTLAS.AccelerationStructure);

        return std::make_pair(outAccel, outBuildInfo);

    }

    AllocatedBuffer VulrayDevice::CreateScratchBufferTLAS(TLASBuildInfo &buildInfo)
    {
        //Get the scratch buffer size
        uint32_t scratchSize = buildInfo.BuildSizes.buildScratchSize;

        //Create the scratch buffer
        AllocatedBuffer scratchBuffer = CreateBuffer(scratchSize, 0,
            vk::BufferUsageFlagBits::eStorageBuffer,
            mAccelProperties.minAccelerationStructureScratchOffsetAlignment);

        //Set the scratch buffer
        buildInfo.BuildGeometryInfo.setScratchData(scratchBuffer.DevAddress);
        return scratchBuffer;
    }
    void VulrayDevice::AddAccelerationBuildBarrier(vk::CommandBuffer cmdBuf)
    {
        //accel build barrier for for next build
        auto barrier = vk::MemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
            .setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR);
        
        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, 
            (vk::DependencyFlagBits)0,
            1, &barrier, 0, nullptr, 0, nullptr);
    }

    
    void VulrayDevice::DestroyBLAS(BLASHandle& blas)
    {
        mDevice.destroyAccelerationStructureKHR(blas.AccelerationStructure, nullptr, mDynLoader);
        DestroyBuffer(blas.BLASBuffer);
    }

    void VulrayDevice::DestroyTLAS(TLASHandle &tlas)
    {
        mDevice.destroyAccelerationStructureKHR(tlas.AccelerationStructure, nullptr, mDynLoader);
        DestroyBuffer(tlas.TLASBuffer);
    }

    void VulrayDevice::DestroyAccelerationStructure(const vk::AccelerationStructureKHR &accel)
    {
        mDevice.destroyAccelerationStructureKHR(accel, nullptr, mDynLoader);
    }
    vk::AccelerationStructureGeometryDataKHR ConvertToVulkanGeometry(const GeometryData &geom)
    {
        vk::AccelerationStructureGeometryDataKHR outGeom = {};

        switch (geom.Type)
        {
        case vk::GeometryTypeKHR::eTriangles:
        {
            return outGeom.setTriangles(vk::AccelerationStructureGeometryTrianglesDataKHR()
                .setVertexFormat(geom.VertexFormat)
                .setVertexData(geom.DataAddresses.VertexDevAddress)
                .setVertexStride(geom.Stride)
                .setMaxVertex(geom.PrimitiveCount * 3) // 3 vertices per triangle
                .setIndexType(geom.IndexFormat)
                .setIndexData(geom.DataAddresses.IndexDevAddress)
                .setTransformData(geom.DataAddresses.TransformBuffer));
        }
        case vk::GeometryTypeKHR::eAabbs:
        {
            return outGeom.setAabbs(vk::AccelerationStructureGeometryAabbsDataKHR()
                .setData(geom.DataAddresses.AABBDevAddress)
                .setStride(geom.Stride));
        }
        }
        return outGeom;
    }
}