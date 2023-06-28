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
        outBuildInfo.Geometries = std::make_unique_for_overwrite<vk::AccelerationStructureGeometryKHR[]>(geomSize); 
        outBuildInfo.GeometryCount = geomSize;

        outBuildInfo.Ranges = std::make_unique_for_overwrite<vk::AccelerationStructureBuildRangeInfoKHR[]>(geomSize);
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
        outAccel.Buffer = CreateBuffer(outBuildInfo.BuildSizes.accelerationStructureSize, 
            0, // no flags for VMA
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);
        

        //Create the acceleration structure
        auto createInfo = vk::AccelerationStructureCreateInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setBuffer(outAccel.Buffer.Buffer)
            .setSize(outBuildInfo.BuildSizes.accelerationStructureSize);
        
        outAccel.AccelerationStructure = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

        //Get the address of the acceleration structure, because it may vary from getBufferDeviceAddress(...)
        outAccel.Buffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR()
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
        assert(updateInfo.NewGeometryAddresses.size() == updateInfo.SourceBuildInfo.GeometryCount && 
        "The number of new geometry addresses must match the number of geometries in the source build info");

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

    CompactionRequest VulrayDevice::RequestCompaction(const std::vector<BLASHandle*>& sourceBLAS)
    {
        CompactionRequest outRequest = {};

        auto createInfo = vk::QueryPoolCreateInfo()
            .setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR)
            .setQueryCount(sourceBLAS.size());
        for (auto& blas : sourceBLAS)
            outRequest.SourceBLAS.push_back(blas->AccelerationStructure);
        outRequest.CompactionQueryPool = mDevice.createQueryPool(createInfo);

        return outRequest;
    }

    std::vector<uint64_t> VulrayDevice::GetCompactionSizes(CompactionRequest& request, vk::CommandBuffer cmdBuf)
    {
        uint32_t blasCount = request.SourceBLAS.size();

        auto [result, values] = mDevice.getQueryPoolResults<uint64_t>(request.CompactionQueryPool,
            0, blasCount, sizeof(uint64_t) * blasCount, sizeof(uint64_t));

        if(result == vk::Result::eSuccess)
        {
            // Destroy the query pool, we don't need it anymore
            mDevice.destroyQueryPool(request.CompactionQueryPool);
            request.CompactionQueryPool = nullptr;

            return values;
        }

        cmdBuf.resetQueryPool(request.CompactionQueryPool, 0, blasCount);
        
        cmdBuf.writeAccelerationStructuresPropertiesKHR(request.SourceBLAS,
            vk::QueryType::eAccelerationStructureCompactedSizeKHR,
            request.CompactionQueryPool, 0, mDynLoader);

        return std::vector<uint64_t>(); // return empty vector
    }

    std::vector<BLASHandle> VulrayDevice::CompactBLAS(CompactionRequest& request, const std::vector<uint64_t>& sizes, vk::CommandBuffer cmdBuf)
    {
        uint32_t blasCount = request.SourceBLAS.size();
        std::vector<BLASHandle> newBLASToReturn(blasCount);

        for (uint32_t i = 0; i < blasCount; i++)
        {
            if(sizes[i] == 0)
                continue;
            // Create buffer
            AllocatedBuffer compactBuffer = CreateBuffer(sizes[i], 0, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

            // Create the compacted acceleration structure
            auto createInfo = vk::AccelerationStructureCreateInfoKHR()
                .setSize(sizes[i])
                .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                .setBuffer(compactBuffer.Buffer);

            auto compactAccel = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

            auto copyInfo = vk::CopyAccelerationStructureInfoKHR()
				.setSrc(request.SourceBLAS[i])
				.setDst(compactAccel)
				.setMode(vk::CopyAccelerationStructureModeKHR::eCompact);

            cmdBuf.copyAccelerationStructureKHR(copyInfo, mDynLoader);

            // Set the new acceleration structure
            newBLASToReturn[i].AccelerationStructure = compactAccel;
            newBLASToReturn[i].Buffer = compactBuffer;
            auto addressInfo = vk::AccelerationStructureDeviceAddressInfoKHR()
                .setAccelerationStructure(newBLASToReturn[i].AccelerationStructure);
            newBLASToReturn[i].Buffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(addressInfo, mDynLoader);
        }
        return newBLASToReturn;
    }

    std::vector<BLASHandle> VulrayDevice::CompactBLAS(CompactionRequest& request, const std::vector<uint64_t>& sizes,
        std::vector<BLASHandle*> oldBLAS, vk::CommandBuffer cmdBuf)
    {
        uint32_t blasCount = request.SourceBLAS.size();
        std::vector<BLASHandle> oldBLASToReturn(blasCount);

        for (uint32_t i = 0; i < blasCount; i++)
        {
            if(sizes[i] == 0)
                continue;
            // Create buffer
            AllocatedBuffer compactBuffer = CreateBuffer(sizes[i], 0, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

            // Create the compacted acceleration structure
            auto createInfo = vk::AccelerationStructureCreateInfoKHR()
                .setSize(sizes[i])
                .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                .setBuffer(compactBuffer.Buffer);

            auto compactAccel = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

            auto copyInfo = vk::CopyAccelerationStructureInfoKHR()
                .setSrc(request.SourceBLAS[i])
                .setDst(compactAccel)
                .setMode(vk::CopyAccelerationStructureModeKHR::eCompact);

            cmdBuf.copyAccelerationStructureKHR(copyInfo, mDynLoader);

            // store the old acceleration structure
            oldBLASToReturn[i].AccelerationStructure = oldBLAS[i]->AccelerationStructure;
            oldBLASToReturn[i].Buffer = oldBLAS[i]->Buffer;
            auto addressInfo = vk::AccelerationStructureDeviceAddressInfoKHR()
				.setAccelerationStructure(oldBLASToReturn[i].AccelerationStructure);
            oldBLASToReturn[i].Buffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(addressInfo, mDynLoader);

            // replace 
            oldBLAS[i]->AccelerationStructure = compactAccel;
            oldBLAS[i]->Buffer = compactBuffer;
        }
        return oldBLASToReturn;
    }

    AllocatedBuffer VulrayDevice::CreateScratchBufferFromBuildInfos(std::vector<BLASBuildInfo>& buildInfos)
    {
        uint32_t scratchSize = GetScratchBufferSize(buildInfos);

        auto outScratchBuffer = CreateScratchBuffer(scratchSize);

        BindScratchBufferToBuildInfos(outScratchBuffer, buildInfos);
        
        return outScratchBuffer;
    }

    AllocatedBuffer VulrayDevice::CreateScratchBufferFromBuildInfo(BLASBuildInfo &buildInfo)
    {
        uint32_t scratchSize = buildInfo.BuildGeometryInfo.mode == vk::BuildAccelerationStructureModeKHR::eBuild ?
            buildInfo.BuildSizes.buildScratchSize : buildInfo.BuildSizes.updateScratchSize;

        auto outScratchBuffer = CreateScratchBuffer(scratchSize);

        BindScratchAdressToBuildInfo(outScratchBuffer.DevAddress, buildInfo);

        return outScratchBuffer;
    }

    void VulrayDevice::BindScratchAdressToBuildInfo(vk::DeviceAddress scratchAddr, BLASBuildInfo &buildInfo)
    {
        buildInfo.BuildGeometryInfo.setScratchData(AlignUp(scratchAddr, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment));
    }

    uint32_t VulrayDevice::GetScratchBufferSize(const std::vector<BLASBuildInfo>& buildInfos)
    {
        uint32_t scratchSize = 0;
        for (auto& info : buildInfos)
        {
            vk::BuildAccelerationStructureModeKHR mode = info.BuildGeometryInfo.mode;
            if(mode == vk::BuildAccelerationStructureModeKHR::eBuild)
                scratchSize += AlignUp(info.BuildSizes.buildScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
            else
                scratchSize += AlignUp(info.BuildSizes.updateScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
        }

        //Mode
        return scratchSize;
    }

    void VulrayDevice::BindScratchBufferToBuildInfos(const vr::AllocatedBuffer& buffer, std::vector<BLASBuildInfo>& buildInfos)
    {
        vk::DeviceAddress scratchDataAddr = buffer.DevAddress;
        for (auto& info : buildInfos)
        {
            vk::BuildAccelerationStructureModeKHR mode = info.BuildGeometryInfo.mode;
            BindScratchAdressToBuildInfo(scratchDataAddr, info);
            if(mode == vk::BuildAccelerationStructureModeKHR::eBuild)
                scratchDataAddr += AlignUp(info.BuildSizes.buildScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
            else
                scratchDataAddr += AlignUp(info.BuildSizes.updateScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
		}
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
            .setGeometryCount(1); // MUST BE 1 (https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAccelerationStructureBuildGeometryInfoKHR.html#VUID-VkAccelerationStructureBuildGeometryInfoKHR-type-03790)

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
            &info.MaxInstanceCount, // max number of instances/primitives in the geometry
            &outBuildInfo.BuildSizes,
            mDynLoader); 
        
        //Create the buffer for the acceleration structure
        outAccel.Buffer = CreateBuffer(outBuildInfo.BuildSizes.accelerationStructureSize, 
            0, // no flags for VMA
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);
        

        //Create the acceleration structure
        auto createInfo = vk::AccelerationStructureCreateInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setBuffer(outAccel.Buffer.Buffer)
            .setSize(outBuildInfo.BuildSizes.accelerationStructureSize);
        
        outAccel.AccelerationStructure = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

        outAccel.Buffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR()
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
            .setBuffer(outAccel.Buffer.Buffer)
            .setSize(outBuildInfo.BuildSizes.accelerationStructureSize);

        outAccel.AccelerationStructure = mDevice.createAccelerationStructureKHR(createInfo, nullptr, mDynLoader);

        outAccel.Buffer.DevAddress = mDevice.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR()
            .setAccelerationStructure(outAccel.AccelerationStructure), mDynLoader);

        outBuildInfo.BuildGeometryInfo.setDstAccelerationStructure(outAccel.AccelerationStructure);

        // destroy the old one
        if(destroyOld)
            DestroyAccelerationStructure(oldTLAS.AccelerationStructure);

        return std::make_pair(outAccel, outBuildInfo);

    }

    void VulrayDevice::BindScratchAdressToBuildInfo(vk::DeviceAddress scratchAddr, TLASBuildInfo& buildInfo)
    {
        buildInfo.BuildGeometryInfo.setScratchData(AlignUp(scratchAddr, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment));
    }

    void VulrayDevice::BindScratchBufferToBuildInfos(const vr::AllocatedBuffer& buffer, std::vector<TLASBuildInfo>& buildInfos)
    {
        vk::DeviceAddress scratchDataAddr = buffer.DevAddress;
        for (auto& info : buildInfos)
        {
            vk::BuildAccelerationStructureModeKHR mode = info.BuildGeometryInfo.mode;
            BindScratchAdressToBuildInfo(scratchDataAddr, info);
            if(mode == vk::BuildAccelerationStructureModeKHR::eBuild)
                scratchDataAddr += AlignUp(info.BuildSizes.buildScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
            else
                scratchDataAddr += AlignUp(info.BuildSizes.updateScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
		}
    }


    uint32_t VulrayDevice::GetScratchBufferSize(const std::vector<TLASBuildInfo>& buildInfos)
    {
        uint32_t scratchSize = 0;
        for (auto& info : buildInfos)
        {
            vk::BuildAccelerationStructureModeKHR mode = info.BuildGeometryInfo.mode;
            if(mode == vk::BuildAccelerationStructureModeKHR::eBuild)
                scratchSize += AlignUp(info.BuildSizes.buildScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
            else
                scratchSize += AlignUp(info.BuildSizes.updateScratchSize, (uint64_t)mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
        }
        return scratchSize;
    }

    AllocatedBuffer VulrayDevice::CreateScratchBufferFromBuildInfos(std::vector<TLASBuildInfo> &buildInfos)
    {
        uint32_t scratchSize = GetScratchBufferSize(buildInfos);

        auto outScratchBuffer = CreateScratchBuffer(scratchSize);

        BindScratchBufferToBuildInfos(outScratchBuffer, buildInfos);

        return outScratchBuffer;
    }

    AllocatedBuffer VulrayDevice::CreateScratchBufferFromBuildInfo(TLASBuildInfo &buildInfo)
    {
        uint32_t scratchSize = buildInfo.BuildGeometryInfo.mode == vk::BuildAccelerationStructureModeKHR::eBuild ?
            buildInfo.BuildSizes.buildScratchSize : buildInfo.BuildSizes.updateScratchSize;

        auto outScratchBuffer = CreateScratchBuffer(scratchSize);

        BindScratchAdressToBuildInfo(outScratchBuffer.DevAddress, buildInfo);

        return outScratchBuffer;
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

    void VulrayDevice::DestroyBLAS(std::vector<BLASHandle>& blas)
    {
        for (auto& b : blas)
        {
            mDevice.destroyAccelerationStructureKHR(b.AccelerationStructure, nullptr, mDynLoader);
            DestroyBuffer(b.Buffer);
        }
    }

    void VulrayDevice::DestroyBLAS(BLASHandle& blas)
    {
        mDevice.destroyAccelerationStructureKHR(blas.AccelerationStructure, nullptr, mDynLoader);
        DestroyBuffer(blas.Buffer);
    }

    void VulrayDevice::DestroyTLAS(TLASHandle &tlas)
    {
        mDevice.destroyAccelerationStructureKHR(tlas.AccelerationStructure, nullptr, mDynLoader);
        DestroyBuffer(tlas.Buffer);
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
                .setTransformData(geom.DataAddresses.TransformDevAddress));
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