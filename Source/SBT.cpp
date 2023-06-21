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

        const uint32_t rgenCount = sbt.RayGenIndices.size();
        const uint32_t missCount = sbt.MissIndices.size();
        const uint32_t hitCount  = sbt.HitGroupIndices.size();
        const uint32_t callCount = sbt.CallableIndices.size();

        const uint32_t handleSize = mRayTracingProperties.shaderGroupHandleSize;
        const uint32_t handleSizeAligned = AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        uint32_t rgenSize = AlignUp(sbt.RayGenShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t missSize = AlignUp(sbt.MissShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t hitSize  = AlignUp(sbt.HitGroupRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t callSize = AlignUp(sbt.CallableShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);


        //Create all buffers for the SBT
        if(sbt.RayGenIndices.size() || sbt.ReserveRayGenGroups)
            outSBT.RayGenBuffer = CreateBuffer(rgenSize * (rgenCount + sbt.ReserveRayGenGroups), 
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                mRayTracingProperties.shaderGroupBaseAlignment);
        if(sbt.MissIndices.size() || sbt.ReserveMissGroups)
            outSBT.MissBuffer = CreateBuffer(missSize * (missCount + sbt.ReserveMissGroups),
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                mRayTracingProperties.shaderGroupBaseAlignment);
        if(sbt.HitGroupIndices.size() || sbt.ReserveHitGroups)
            outSBT.HitGroupBuffer = CreateBuffer(hitSize * (hitCount + sbt.ReserveHitGroups),
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                mRayTracingProperties.shaderGroupBaseAlignment);
        if(sbt.CallableIndices.size() || sbt.ReserveCallableGroups)
            outSBT.CallableBuffer = CreateBuffer(callSize * (callCount + sbt.ReserveCallableGroups),
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                mRayTracingProperties.shaderGroupBaseAlignment);


        // For filling the stride and size of the regions, we don't want to set stride when there is no shader of that type.
        // We didn't do this earlier because we needed to know the size of the shader group handles to reserve space for them.
        if (rgenCount == 0)
            rgenSize = 0;
        if (missCount == 0)
            missSize = 0;
        if (hitCount == 0)
            hitSize = 0;
        if (callCount == 0)
            callSize = 0;

        // fill in offsets for all shader groups
        if(rgenCount > 0)
            outSBT.RayGenRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.RayGenBuffer.DevAddress)
                .setStride(rgenSize)
                .setSize(rgenSize * rgenCount);
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


        uint8_t* opaqueHandle = new uint8_t[handleSizeAligned];

        //copy records and shader handles into the SBT buffer
        uint8_t* rgenData = rgenSize > 0 ? (uint8_t*)MapBuffer(outSBT.RayGenBuffer) : nullptr;
        for(uint32_t i = 0; rgenData && i < rgenCount; i++)
        {
            uint32_t shaderIndex = sbt.RayGenIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(rgenData + (i * rgenSize), opaqueHandle, handleSizeAligned);
        }
        uint8_t* missData = missSize > 0 ? (uint8_t*)MapBuffer(outSBT.MissBuffer) : nullptr;
        for(uint32_t i = 0; missData && i < missCount; i++)
        {
            uint32_t shaderIndex = sbt.MissIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(missData + (missSize * i), opaqueHandle, handleSizeAligned);
        }
        uint8_t* hitData = hitSize > 0 ? (uint8_t*)MapBuffer(outSBT.HitGroupBuffer) : nullptr;
        for(uint32_t i = 0; hitData && i < hitCount; i++)
        {
            uint32_t shaderIndex = sbt.HitGroupIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(hitData + (hitSize * i), opaqueHandle, handleSizeAligned);
        }
        uint8_t* callData = callSize > 0 ? (uint8_t*)MapBuffer(outSBT.CallableBuffer) : nullptr;
        for(uint32_t i = 0; callData && i < callCount; i++)
        {
            uint32_t shaderIndex = sbt.CallableIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, opaqueHandle);
            //copy shader handle
            memcpy(callData + (callSize * i), opaqueHandle, handleSizeAligned);
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
        
        delete[] opaqueHandle;

        return outSBT;
    }

    bool VulrayDevice::ExtendSBT(vk::Pipeline pipeline, SBTBuffer &buffer, const ShaderBindingTableInfo &sbtInfo)
    {
        if(!CanSBTFitShaders(buffer, sbtInfo))
            return false;

        const uint32_t rgenSize = AlignUp(sbtInfo.RayGenShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t missSize = AlignUp(sbtInfo.MissShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t hitSize  = AlignUp(sbtInfo.HitGroupRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t callSize = AlignUp(sbtInfo.CallableShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
    
        const uint32_t handleSize = mRayTracingProperties.shaderGroupHandleSize;
        const uint32_t handleSizeAligned = AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        // Check where to start copying the new shaders, if there is no stride we start at 0, because the shaders haveb't being added
        const uint32_t rgenStart = buffer.RayGenRegion.stride == 0 ? 0 : buffer.RayGenRegion.size / rgenSize;
        const uint32_t missStart = buffer.MissRegion.stride == 0 ? 0 : buffer.MissRegion.size / missSize;
        const uint32_t hitStart  = buffer.HitGroupRegion.stride == 0 ? 0 : buffer.HitGroupRegion.size / hitSize;
        const uint32_t callStart = buffer.CallableRegion.stride == 0 ? 0 : buffer.CallableRegion.size / callSize;

        uint8_t* opaqueHandle = new uint8_t[handleSizeAligned];

        // Copy the new shaders into the buffer
        uint8_t* rgenData = rgenStart == sbtInfo.RayGenIndices.size() ? nullptr : (uint8_t*)MapBuffer(buffer.RayGenBuffer);
        for(uint32_t i = rgenStart; rgenData && i < sbtInfo.RayGenIndices.size(); i++)
        {
            uint32_t shaderIndex = sbtInfo.RayGenIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, rgenData + (rgenSize * (rgenStart + i)));
            memcpy(rgenData + (rgenSize * (rgenStart + i)), opaqueHandle, handleSizeAligned);
        }
        uint8_t* missData = missStart == sbtInfo.MissIndices.size() ? nullptr : (uint8_t*)MapBuffer(buffer.MissBuffer);
        for(uint32_t i = missStart; missData && i < sbtInfo.MissIndices.size(); i++)
        {
            uint32_t shaderIndex = sbtInfo.MissIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, missData + (missSize * (missStart + i)));
            memcpy(missData + (missSize * (missStart + i)), opaqueHandle, handleSizeAligned);    
        }
        uint8_t* hitData = hitStart == sbtInfo.HitGroupIndices.size() ? nullptr : (uint8_t*)MapBuffer(buffer.HitGroupBuffer);
        for(uint32_t i = hitStart; hitData && i < sbtInfo.HitGroupIndices.size(); i++)
        {
            uint32_t shaderIndex = sbtInfo.HitGroupIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, hitData + (hitSize * (hitStart + i)));
            memcpy(hitData + (hitSize * (hitStart + i)), opaqueHandle, handleSizeAligned);
        }
        uint8_t* callData = callStart == sbtInfo.CallableIndices.size() ? nullptr : (uint8_t*)MapBuffer(buffer.CallableBuffer);
        for(uint32_t i = callStart; callData && i < sbtInfo.CallableIndices.size(); i++)
        {
            uint32_t shaderIndex = sbtInfo.CallableIndices[i];
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, callData + (callSize * (callStart + i)));
            memcpy(callData + (callSize * (callStart + i)), opaqueHandle, handleSizeAligned);
        }

        //unmap all the buffers
        if(rgenData)
            UnmapBuffer(buffer.RayGenBuffer);
        if(missData)
            UnmapBuffer(buffer.MissBuffer);
        if(hitData)
            UnmapBuffer(buffer.HitGroupBuffer);
        if(callData)
            UnmapBuffer(buffer.CallableBuffer);

        //update the regions
        if(rgenStart != sbtInfo.RayGenIndices.size())
        {
            // Add the size of the new shaders to the region
            // we don't want to set it to the size of the buffer because we might have reserved space for more shaders
            buffer.RayGenRegion.size += rgenSize * (sbtInfo.RayGenIndices.size() - rgenStart);
            buffer.RayGenRegion.stride = rgenSize;
        }
        if(missStart != sbtInfo.MissIndices.size())
        {
            buffer.MissRegion.size += missSize * (sbtInfo.MissIndices.size() - missStart);
            buffer.MissRegion.stride = missSize;
        }
        if(hitStart != sbtInfo.HitGroupIndices.size())
        {
            buffer.HitGroupRegion.size += hitSize * (sbtInfo.HitGroupIndices.size() - hitStart);
            buffer.HitGroupRegion.stride = hitSize;
        }
        if(callStart != sbtInfo.CallableIndices.size())
        {
            buffer.CallableRegion.size += callSize * (sbtInfo.CallableIndices.size() - callStart);
            buffer.CallableRegion.stride = callSize;
        }

        delete[] opaqueHandle;

        return true;
    }

    bool VulrayDevice::CanSBTFitShaders(SBTBuffer &buffer, const ShaderBindingTableInfo &sbtInfo)
    {
        bool extendable = true;

        const uint32_t rgenSize = AlignUp(sbtInfo.RayGenShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t missSize = AlignUp(sbtInfo.MissShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t hitSize  = AlignUp(sbtInfo.HitGroupRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t callSize = AlignUp(sbtInfo.CallableShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        const uint32_t rgenBytesNeeded = sbtInfo.RayGenIndices.size() * rgenSize;
        const uint32_t missBytesNeeded = sbtInfo.MissIndices.size() * missSize;
        const uint32_t hitBytesNeeded  = sbtInfo.HitGroupIndices.size() * hitSize;
        const uint32_t callBytesNeeded = sbtInfo.CallableIndices.size() * callSize;

        if(rgenBytesNeeded > buffer.RayGenBuffer.Size)
            return false;
        if(missBytesNeeded > buffer.MissBuffer.Size)
            return false;
        if(hitBytesNeeded > buffer.HitGroupBuffer.Size) 
            return false;
        if(callBytesNeeded > buffer.CallableBuffer.Size)
            return false;
            
        return extendable;
    }

    void VulrayDevice::DestroySBTBuffer(SBTBuffer &buffer)
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