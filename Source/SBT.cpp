#include "Vulray/VulrayDevice.h"
#include "Vulray/SBT.h"

namespace vr
{
    
    std::vector<uint8_t> VulrayDevice::GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup, uint32_t groupCount)
    {
        uint32_t alignedSize = AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t size = alignedSize * groupCount;
        std::vector<uint8_t> handles(size);

        auto result = mDevice.getRayTracingShaderGroupHandlesKHR(pipeline, firstGroup, groupCount, size, handles.data(), mDynLoader);
        if(result != vk::Result::eSuccess)
        {
            VULRAY_LOG_ERROR("GetHandlesForSBTBuffer: Failed to get ray tracing shader group handles");
        }
        return std::move(handles);
    }

    void VulrayDevice::GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup, uint32_t groupCount, void *data)
    {
        uint32_t alignedSize = AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t size = alignedSize * groupCount;

        auto result = mDevice.getRayTracingShaderGroupHandlesKHR(pipeline, firstGroup, groupCount, size, data, mDynLoader);
        if(result != vk::Result::eSuccess)
        {
            VULRAY_LOG_ERROR("GetHandlesForSBTBuffer: Failed to get ray tracing shader group handles");
        }

    }

    void VulrayDevice::WriteToSBT(SBTBuffer sbtBuf, ShaderGroup group, uint32_t groupIndex, void *data, uint32_t dataSize, void *mappedData)
    {
        AllocatedBuffer* buffer = nullptr;
        vk::StridedDeviceAddressRegionKHR* addressRegion;
        switch (group)
        {
        case ShaderGroup::RayGen: buffer = &sbtBuf.RayGenBuffer; addressRegion = &sbtBuf.RayGenRegion; break;
        case ShaderGroup::Miss: buffer = &sbtBuf.MissBuffer; addressRegion = &sbtBuf.MissRegion; break;
        case ShaderGroup::HitGroup: buffer = &sbtBuf.HitGroupBuffer; addressRegion = &sbtBuf.HitGroupRegion; break;
        case ShaderGroup::Callable: buffer = &sbtBuf.CallableBuffer; addressRegion = &sbtBuf.CallableRegion; break;
        }
        if(!buffer)
        {
            VULRAY_LOG_ERROR("WriteToSBT: Invalid shader group");
            return;
        }

        // Offset to the start of the requested group and apply the opaque handle size for the SBT
        uint32_t offset = (groupIndex * addressRegion->stride) + mRayTracingProperties.shaderGroupHandleSize;

        //make sure the data size is not too large
        if(offset + dataSize > addressRegion->size)
        {
            VULRAY_LOG_ERROR("WriteToSBT: Data size is too large for shader group");
            return;
        }

        if(mappedData)
        {
            memcpy((uint8_t*)mappedData + offset, data, dataSize); // if we have a mapped buffer, just copy the data
        }
        else
        {
            //else we update the buffer with the data
            UpdateBuffer(*buffer, data, dataSize, offset);
        }
    }

    SBTBuffer VulrayDevice::CreateSBT(vk::Pipeline pipeline, const ShaderBindingTableInfo& sbt)
    {
        SBTBuffer outSBT;

        uint32_t rgenCount = sbt.RayGenIndices.size();
        uint32_t missCount = sbt.MissIndices.size();
        uint32_t hitCount = sbt.HitGroupIndices.size();
        uint32_t callCount = sbt.CallableIndices.size();

        uint32_t handleSize = mRayTracingProperties.shaderGroupHandleSize;
        uint32_t handleSizeAligned = AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        uint32_t rgenSize = AlignUp(sbt.RayGenShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t missSize = AlignUp(sbt.MissShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t hitSize = AlignUp(sbt.HitGroupRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t callSize = AlignUp(sbt.CallableShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);


        if (rgenCount == 0)
            rgenSize = 0;
        if (missCount == 0)
            missSize = 0;
        if (hitCount == 0)
            hitSize = 0;
        if (callCount == 0)
            callSize = 0;

        //Create all buffers for the SBT
        if(rgenSize > 0)
            outSBT.RayGenBuffer = CreateBuffer(rgenSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR, mRayTracingProperties.shaderGroupBaseAlignment);
        if(missSize > 0)
            outSBT.MissBuffer = CreateBuffer(missSize * missCount, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR, mRayTracingProperties.shaderGroupBaseAlignment);
        if(hitSize > 0)
            outSBT.HitGroupBuffer = CreateBuffer(hitSize * hitCount, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR, mRayTracingProperties.shaderGroupBaseAlignment);
        if(callSize > 0)
            outSBT.CallableBuffer = CreateBuffer(callSize * callCount, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR, mRayTracingProperties.shaderGroupBaseAlignment);
        
        //map all the buffers


        // fill in offsets for all shader groups

        if(rgenCount > 0)
            outSBT.RayGenRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.RayGenBuffer.DevAddress)
                .setStride(rgenSize)
                .setSize(rgenSize); // only one ray gen shader
        if(missCount > 0)
            outSBT.MissRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.MissBuffer.DevAddress)
                .setStride(missSize)
                .setSize(missSize * missCount);
        if(hitCount > 0)
            outSBT.HitGroupRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.HitGroupBuffer.DevAddress)
                .setStride(hitSize)
                .setSize(hitSize * hitCount);
        if(callCount > 0)
            outSBT.CallableRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.CallableBuffer.DevAddress)
                .setStride(callSize)
                .setSize(callSize * callCount);


        uint8_t* opaqueHandle = new uint8_t[mRayTracingProperties.shaderGroupHandleSize];
        
        //copy records and shader handles into the SBT buffer
        void* rgenData = MapBuffer(outSBT.RayGenBuffer);
        for(uint32_t i = 0; rgenData && i < rgenCount; i++)
        {
            uint32_t shaderIndex = sbt.RayGenIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(rgenData, opaqueHandle, mRayTracingProperties.shaderGroupHandleSize);
        }
        uint8_t* missData = missSize > 0 ? (uint8_t*)MapBuffer(outSBT.MissBuffer) : nullptr;
        for(uint32_t i = 0; missData && i < missCount; i++)
        {
            uint32_t shaderIndex = sbt.MissIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(missData, opaqueHandle, mRayTracingProperties.shaderGroupHandleSize);
        }
        uint8_t* hitData = hitSize > 0 ? (uint8_t*)MapBuffer(outSBT.HitGroupBuffer) : nullptr;
        for(uint32_t i = 0; hitData && i < hitCount; i++)
        {
            uint32_t shaderIndex = sbt.HitGroupIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(hitData, opaqueHandle, mRayTracingProperties.shaderGroupHandleSize);
        }
        uint8_t* callData = callSize > 0 ? (uint8_t*)MapBuffer(outSBT.CallableBuffer) : nullptr;
        for(uint32_t i = 0; callData && i < callCount; i++)
        {
            uint32_t shaderIndex = sbt.CallableIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(callData, opaqueHandle, mRayTracingProperties.shaderGroupHandleSize);
        }

        //unmap all the buffers
        if(rgenData)
            UnmapBuffer(outSBT.RayGenBuffer);
        if(missData)
            UnmapBuffer(outSBT.MissBuffer);
        if(hitData)
            UnmapBuffer(outSBT.HitGroupBuffer);
        if(callData)
            UnmapBuffer(outSBT.CallableBuffer);
        
        



        return outSBT;
    }

    void VulrayDevice::DestroySBTBuffer(SBTBuffer& buffer)
    {
        //destroy all the buffers if they were created
        if(buffer.RayGenBuffer.Buffer)
            DestroyBuffer(buffer.RayGenBuffer);
        if(buffer.MissBuffer.Buffer)
            DestroyBuffer(buffer.MissBuffer);
        if(buffer.HitGroupBuffer.Buffer)
            DestroyBuffer(buffer.HitGroupBuffer);
        if(buffer.CallableBuffer.Buffer)
            DestroyBuffer(buffer.CallableBuffer);
        buffer.RayGenRegion = vk::StridedDeviceAddressRegionKHR();
        buffer.MissRegion = vk::StridedDeviceAddressRegionKHR();
        buffer.HitGroupRegion = vk::StridedDeviceAddressRegionKHR();
        buffer.CallableRegion = vk::StridedDeviceAddressRegionKHR();
    }
}