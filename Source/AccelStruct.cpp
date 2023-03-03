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


    AllocatedBuffer VulrayDevice::BuildBLAS(std::vector<BLASBuildInfo> &buildInfos, vk::CommandBuffer cmdBuf)
    {
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos;
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
        pBuildRangeInfos.reserve(buildInfos.size());
        buildGeometryInfos.reserve(buildInfos.size());
        uint32_t scratchSize = 0;
        // loop over build infos and get scratch sizes
        for(uint32_t i = 0; i < buildInfos.size(); i++)
        {
            scratchSize += buildInfos[i].BuildSizes.buildScratchSize;
            pBuildRangeInfos.push_back(buildInfos[i].Ranges.get());
            buildGeometryInfos.push_back(buildInfos[i].BuildGeometryInfo);
        }

        AllocatedBuffer scratchBuffer = CreateBuffer(scratchSize, 0,
            vk::BufferUsageFlagBits::eStorageBuffer,
            mAccelProperties.minAccelerationStructureScratchOffsetAlignment);

        // loop over build infos and set scratch buffers
        // this will offset the scratch buffer so that each build info has its own region of the scratch buffer
        vk::DeviceAddress scratchDataAddr = scratchBuffer.DevAddress;

        for (uint32_t i = 0; i < buildInfos.size(); i++)
        {
			buildGeometryInfos[i].setScratchData(scratchBuffer.DevAddress);
            scratchDataAddr += buildInfos[i].BuildSizes.buildScratchSize;
		}
        
        //Build the acceleration structures
        cmdBuf.buildAccelerationStructuresKHR(buildInfos.size(), buildGeometryInfos.data(), pBuildRangeInfos.data(), mDynLoader);
        

        return scratchBuffer;
    }

    //--------------------------------------------------------------------------------------
    // TLAS FUCNTIONS
    //--------------------------------------------------------------------------------------

    std::pair<TLASHandle, TLASBuildInfo> VulrayDevice::CreateTLAS(const TLASCreateInfo& info)
    {
        
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

    vk::AccelerationStructureGeometryDataKHR ConvertToVulkanGeometry(const GeometryData& geom)
    {
        vk::AccelerationStructureGeometryDataKHR outGeom = {};

        switch (geom.Type)
        {
        case vk::GeometryTypeKHR::eTriangles:
        {
            return outGeom.setTriangles(vk::AccelerationStructureGeometryTrianglesDataKHR()
                .setVertexFormat(geom.VertexFormat)
                .setVertexData(geom.VertexDevAddress)
                .setVertexStride(geom.Stride)
                .setMaxVertex(geom.PrimitiveCount * 3) // 3 vertices per triangle
                .setIndexType(geom.IndexFormat)
                .setIndexData(geom.IndexDevAddress)
                .setTransformData(geom.TransformBuffer));
        }
        case vk::GeometryTypeKHR::eAabbs:
        {
            return outGeom.setAabbs(vk::AccelerationStructureGeometryAabbsDataKHR()
                .setData(geom.AABBDevAddress)
                .setStride(geom.Stride));
        }
        }
        return outGeom;
    }
}