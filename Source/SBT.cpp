#include "Vulray/SBT.h"

#include "Vulray/VulrayDevice.h"

namespace vr
{

    std::vector<uint8_t> VulrayDevice::GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup,
                                                              uint32_t groupCount)
    {
        uint32_t alignedSize =
            AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t size = alignedSize * groupCount;
        std::vector<uint8_t> handles(size);

        auto result = mDevice.getRayTracingShaderGroupHandlesKHR(pipeline, firstGroup, groupCount, size, handles.data(),
                                                                 mDynLoader);
        if (result != vk::Result::eSuccess)
        {
            VULRAY_LOG_ERROR("GetHandlesForSBTBuffer: Failed to get ray tracing shader group handles");
        }
        return std::move(handles);
    }

    void VulrayDevice::GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup, uint32_t groupCount,
                                              void* data)
    {
        uint32_t alignedSize =
            AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t size = alignedSize * groupCount;

        auto result =
            mDevice.getRayTracingShaderGroupHandlesKHR(pipeline, firstGroup, groupCount, size, data, mDynLoader);
        if (result != vk::Result::eSuccess)
        {
            VULRAY_LOG_ERROR("GetHandlesForSBTBuffer: Failed to get ray tracing shader group handles");
        }
    }

    void VulrayDevice::WriteToSBT(SBTBuffer sbtBuf, ShaderGroup group, uint32_t groupIndex, void* data,
                                  uint32_t dataSize, void* mappedData)
    {
        AllocatedBuffer* buffer = nullptr;
        vk::StridedDeviceAddressRegionKHR* addressRegion;
        switch (group)
        {
        case ShaderGroup::RayGen:
            buffer = &sbtBuf.RayGenBuffer;
            addressRegion = &sbtBuf.RayGenRegion;
            break;
        case ShaderGroup::Miss:
            buffer = &sbtBuf.MissBuffer;
            addressRegion = &sbtBuf.MissRegion;
            break;
        case ShaderGroup::HitGroup:
            buffer = &sbtBuf.HitGroupBuffer;
            addressRegion = &sbtBuf.HitGroupRegion;
            break;
        case ShaderGroup::Callable:
            buffer = &sbtBuf.CallableBuffer;
            addressRegion = &sbtBuf.CallableRegion;
            break;
        }
        if (!buffer)
        {
            VULRAY_LOG_ERROR("WriteToSBT: Invalid shader group");
            return;
        }

        // Offset to the start of the requested group and apply the opaque handle size for the SBT
        uint32_t offset = (groupIndex * addressRegion->stride) + mRayTracingProperties.shaderGroupHandleSize;

        // make sure the data size is not too large
        if (offset + dataSize > addressRegion->size)
        {
            VULRAY_LOG_ERROR("WriteToSBT: Data size is too large for shader group");
            return;
        }

        if (mappedData)
        {
            memcpy((uint8_t*)mappedData + offset, data, dataSize); // if we have a mapped buffer, just copy the data
        }
        else
        {
            // else we update the buffer with the data
            UpdateBuffer(*buffer, data, dataSize, offset);
        }
    }

    SBTBuffer VulrayDevice::CreateSBT(vk::Pipeline pipeline, const SBTInfo& sbt)
    {
        SBTBuffer outSBT;

        const uint32_t rgenCount = sbt.RayGenIndices.size();
        const uint32_t missCount = sbt.MissIndices.size();
        const uint32_t hitCount = sbt.HitGroupIndices.size();
        const uint32_t callCount = sbt.CallableIndices.size();

        const uint32_t groupsCount = rgenCount + missCount + hitCount + callCount;

        const uint32_t handleSize = mRayTracingProperties.shaderGroupHandleSize;

        uint32_t rgenSize =
            AlignUp(sbt.RayGenShaderRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t missSize =
            AlignUp(sbt.MissShaderRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t hitSize =
            AlignUp(sbt.HitGroupRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t callSize =
            AlignUp(sbt.CallableShaderRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        // Create all buffers for the SBT
        if (sbt.RayGenIndices.size() || sbt.ReserveRayGenGroups)
            outSBT.RayGenBuffer = CreateBuffer(
                rgenSize * (rgenCount + sbt.ReserveRayGenGroups),
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, mRayTracingProperties.shaderGroupBaseAlignment);
        if (sbt.MissIndices.size() || sbt.ReserveMissGroups)
            outSBT.MissBuffer = CreateBuffer(
                missSize * (missCount + sbt.ReserveMissGroups),
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, mRayTracingProperties.shaderGroupBaseAlignment);
        if (sbt.HitGroupIndices.size() || sbt.ReserveHitGroups)
            outSBT.HitGroupBuffer = CreateBuffer(
                hitSize * (hitCount + sbt.ReserveHitGroups),
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, mRayTracingProperties.shaderGroupBaseAlignment);
        if (sbt.CallableIndices.size() || sbt.ReserveCallableGroups)
            outSBT.CallableBuffer = CreateBuffer(
                callSize * (callCount + sbt.ReserveCallableGroups),
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, mRayTracingProperties.shaderGroupBaseAlignment);

        // For filling the stride and size of the regions, we don't want to set stride when there is no shader of that
        // type. We didn't do this earlier because we needed to know the size of the shader group handles to reserve
        // space for them.
        if (rgenCount == 0)
            rgenSize = 0;
        if (missCount == 0)
            missSize = 0;
        if (hitCount == 0)
            hitSize = 0;
        if (callCount == 0)
            callSize = 0;

        // fill in offsets for all shader groups
        if (rgenCount > 0)
            outSBT.RayGenRegion = vk::StridedDeviceAddressRegionKHR()
                                      .setDeviceAddress(outSBT.RayGenBuffer.DevAddress)
                                      .setStride(rgenSize)
                                      .setSize(rgenSize * rgenCount);
        if (missCount > 0)
            outSBT.MissRegion = vk::StridedDeviceAddressRegionKHR()
                                    .setDeviceAddress(outSBT.MissBuffer.DevAddress)
                                    .setStride(missSize)
                                    .setSize(missSize * missCount);
        if (hitCount > 0)
            outSBT.HitGroupRegion = vk::StridedDeviceAddressRegionKHR()
                                        .setDeviceAddress(outSBT.HitGroupBuffer.DevAddress)
                                        .setStride(hitSize)
                                        .setSize(hitSize * hitCount);
        if (callCount > 0)
            outSBT.CallableRegion = vk::StridedDeviceAddressRegionKHR()
                                        .setDeviceAddress(outSBT.CallableBuffer.DevAddress)
                                        .setStride(callSize)
                                        .setSize(callSize * callCount);

        const uint8_t* opaqueHandle = new uint8_t[handleSize];

        // copy records and shader handles into the SBT buffer
        uint8_t* rgenData = rgenSize > 0 ? (uint8_t*)MapBuffer(outSBT.RayGenBuffer) : nullptr;
        for (uint32_t i = 0; rgenData && i < rgenCount; i++)
        {
            uint32_t shaderIndex = sbt.RayGenIndices[i];
            const uint8_t* dest = rgenData + (i * rgenSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
            rgenData += rgenSize; // advance to the next record
        }
        uint8_t* missData = missSize > 0 ? (uint8_t*)MapBuffer(outSBT.MissBuffer) : nullptr;
        for (uint32_t i = 0; missData && i < missCount; i++)
        {
            uint32_t shaderIndex = sbt.MissIndices[i];
            const uint8_t* dest = missData + (i * missSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
        }
        uint8_t* hitData = hitSize > 0 ? (uint8_t*)MapBuffer(outSBT.HitGroupBuffer) : nullptr;
        for (uint32_t i = 0; hitData && i < hitCount; i++)
        {
            uint32_t shaderIndex = sbt.HitGroupIndices[i];
            const uint8_t* dest = hitData + (i * hitSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
        }
        uint8_t* callData = callSize > 0 ? (uint8_t*)MapBuffer(outSBT.CallableBuffer) : nullptr;
        for (uint32_t i = 0; callData && i < callCount; i++)
        {
            uint32_t shaderIndex = sbt.CallableIndices[i];
            const uint8_t* dest = callData + (i * callSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
        }

        // unmap all the buffers
        if (rgenData)
            UnmapBuffer(outSBT.RayGenBuffer);
        if (missData)
            UnmapBuffer(outSBT.MissBuffer);
        if (hitData)
            UnmapBuffer(outSBT.HitGroupBuffer);
        if (callData)
            UnmapBuffer(outSBT.CallableBuffer);

        return outSBT;
    }

    bool VulrayDevice::RebuildSBT(vk::Pipeline pipeline, SBTBuffer& buffer, const SBTInfo& sbt)
    {
        if (!CanSBTFitShaders(buffer, sbt))
            return false;

        const uint32_t rgenCount = sbt.RayGenIndices.size();
        const uint32_t missCount = sbt.MissIndices.size();
        const uint32_t hitCount = sbt.HitGroupIndices.size();
        const uint32_t callCount = sbt.CallableIndices.size();

        const uint32_t groupsCount = rgenCount + missCount + hitCount + callCount;

        const uint32_t handleSize =
            AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        const uint32_t rgenSize =
            AlignUp(sbt.RayGenShaderRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t missSize =
            AlignUp(sbt.MissShaderRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t hitSize =
            AlignUp(sbt.HitGroupRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t callSize =
            AlignUp(sbt.CallableShaderRecordSize + handleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        // we have to rewrite opaque handles to all the groups in the SBT, because on some implementations just keeping
        // the old opaque handles and adding new opaque handles to the new added groups doesn't work

        const uint8_t* opaqueHandle = new uint8_t[handleSize];

        // copy records and shader handles into the SBT buffer
        uint8_t* rgenData = rgenSize > 0 ? (uint8_t*)MapBuffer(buffer.RayGenBuffer) : nullptr;
        for (uint32_t i = 0; rgenData && i < rgenCount; i++)
        {
            uint32_t shaderIndex = sbt.RayGenIndices[i];
            const uint8_t* dest = rgenData + (i * rgenSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
            rgenData += rgenSize; // advance to the next record
        }
        uint8_t* missData = missSize > 0 ? (uint8_t*)MapBuffer(buffer.MissBuffer) : nullptr;
        for (uint32_t i = 0; missData && i < missCount; i++)
        {
            uint32_t shaderIndex = sbt.MissIndices[i];
            const uint8_t* dest = missData + (i * missSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
        }
        uint8_t* hitData = hitSize > 0 ? (uint8_t*)MapBuffer(buffer.HitGroupBuffer) : nullptr;
        for (uint32_t i = 0; hitData && i < hitCount; i++)
        {
            uint32_t shaderIndex = sbt.HitGroupIndices[i];
            const uint8_t* dest = hitData + (i * hitSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
        }
        uint8_t* callData = callSize > 0 ? (uint8_t*)MapBuffer(buffer.CallableBuffer) : nullptr;
        for (uint32_t i = 0; callData && i < callCount; i++)
        {
            uint32_t shaderIndex = sbt.CallableIndices[i];
            const uint8_t* dest = callData + (i * callSize);
            GetHandlesForSBTBuffer(pipeline, shaderIndex, 1, (void*)dest);
        }

        // unmap all the buffers
        if (rgenData)
            UnmapBuffer(buffer.RayGenBuffer);
        if (missData)
            UnmapBuffer(buffer.MissBuffer);
        if (hitData)
            UnmapBuffer(buffer.HitGroupBuffer);
        if (callData)
            UnmapBuffer(buffer.CallableBuffer);

        // Some groups may have gotten additional shaders, so we need to update the stride and size of the regions
        // We don't have to worry about the buffer sizes if they don't fit as it is already checked at the beginning of
        // this function

        if (rgenCount > 0)
            buffer.RayGenRegion = vk::StridedDeviceAddressRegionKHR()
                                      .setDeviceAddress(buffer.RayGenBuffer.DevAddress)
                                      .setStride(rgenSize)
                                      .setSize(rgenSize * rgenCount);
        if (missCount > 0)
            buffer.MissRegion = vk::StridedDeviceAddressRegionKHR()
                                    .setDeviceAddress(buffer.MissBuffer.DevAddress)
                                    .setStride(missSize)
                                    .setSize(missSize * missCount);
        if (hitCount > 0)
            buffer.HitGroupRegion = vk::StridedDeviceAddressRegionKHR()
                                        .setDeviceAddress(buffer.HitGroupBuffer.DevAddress)
                                        .setStride(hitSize)
                                        .setSize(hitSize * hitCount);
        if (callCount > 0)
            buffer.CallableRegion = vk::StridedDeviceAddressRegionKHR()
                                        .setDeviceAddress(buffer.CallableBuffer.DevAddress)
                                        .setStride(callSize)
                                        .setSize(callSize * callCount);

        return true;
    }

    void VulrayDevice::CopySBT(SBTBuffer& src, SBTBuffer& dst)
    {
        uint8_t* dstRgenData = dst.RayGenRegion.size > 0 ? (uint8_t*)MapBuffer(dst.RayGenBuffer) : nullptr;
        uint8_t* dstMissData = dst.MissRegion.size > 0 ? (uint8_t*)MapBuffer(dst.MissBuffer) : nullptr;
        uint8_t* dstHitData = dst.HitGroupRegion.size > 0 ? (uint8_t*)MapBuffer(dst.HitGroupBuffer) : nullptr;
        uint8_t* dstCallData = dst.CallableRegion.size > 0 ? (uint8_t*)MapBuffer(dst.CallableBuffer) : nullptr;

        uint8_t* srcRgenData = src.RayGenRegion.size > 0 ? (uint8_t*)MapBuffer(src.RayGenBuffer) : nullptr;
        uint8_t* srcMissData = src.MissRegion.size > 0 ? (uint8_t*)MapBuffer(src.MissBuffer) : nullptr;
        uint8_t* srcHitData = src.HitGroupRegion.size > 0 ? (uint8_t*)MapBuffer(src.HitGroupBuffer) : nullptr;
        uint8_t* srcCallData = src.CallableRegion.size > 0 ? (uint8_t*)MapBuffer(src.CallableBuffer) : nullptr;

        if (dstRgenData && srcRgenData)
            memcpy(dstRgenData, srcRgenData, src.RayGenRegion.size);
        if (dstMissData && srcMissData)
            memcpy(dstMissData, srcMissData, src.MissRegion.size);
        if (dstHitData && srcHitData)
            memcpy(dstHitData, srcHitData, src.HitGroupRegion.size);
        if (dstCallData && srcCallData)
            memcpy(dstCallData, srcCallData, src.CallableRegion.size);

        if (dstRgenData)
            UnmapBuffer(dst.RayGenBuffer);
        if (dstMissData)
            UnmapBuffer(dst.MissBuffer);
        if (dstHitData)
            UnmapBuffer(dst.HitGroupBuffer);
        if (dstCallData)
            UnmapBuffer(dst.CallableBuffer);

        if (srcRgenData)
            UnmapBuffer(src.RayGenBuffer);
        if (srcMissData)
            UnmapBuffer(src.MissBuffer);
        if (srcHitData)
            UnmapBuffer(src.HitGroupBuffer);
        if (srcCallData)
            UnmapBuffer(src.CallableBuffer);
    }

    bool VulrayDevice::CanSBTFitShaders(SBTBuffer& buffer, const SBTInfo& sbtInfo)
    {
        bool extendable = true;

        const uint32_t rgenSize = AlignUp(sbtInfo.RayGenShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize,
                                          mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t missSize = AlignUp(sbtInfo.MissShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize,
                                          mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t hitSize = AlignUp(sbtInfo.HitGroupRecordSize + mRayTracingProperties.shaderGroupHandleSize,
                                         mRayTracingProperties.shaderGroupHandleAlignment);
        const uint32_t callSize =
            AlignUp(sbtInfo.CallableShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize,
                    mRayTracingProperties.shaderGroupHandleAlignment);

        const uint32_t rgenBytesNeeded = sbtInfo.RayGenIndices.size() * rgenSize;
        const uint32_t missBytesNeeded = sbtInfo.MissIndices.size() * missSize;
        const uint32_t hitBytesNeeded = sbtInfo.HitGroupIndices.size() * hitSize;
        const uint32_t callBytesNeeded = sbtInfo.CallableIndices.size() * callSize;

        if (rgenBytesNeeded > buffer.RayGenBuffer.Size)
            return false;
        if (missBytesNeeded > buffer.MissBuffer.Size)
            return false;
        if (hitBytesNeeded > buffer.HitGroupBuffer.Size)
            return false;
        if (callBytesNeeded > buffer.CallableBuffer.Size)
            return false;

        return extendable;
    }

    void VulrayDevice::DestroySBTBuffer(SBTBuffer& buffer)
    {
        // destroy all the buffers if they were created
        if (buffer.RayGenBuffer.Buffer)
            DestroyBuffer(buffer.RayGenBuffer);
        if (buffer.MissBuffer.Buffer)
            DestroyBuffer(buffer.MissBuffer);
        if (buffer.HitGroupBuffer.Buffer)
            DestroyBuffer(buffer.HitGroupBuffer);
        if (buffer.CallableBuffer.Buffer)
            DestroyBuffer(buffer.CallableBuffer);
        buffer.RayGenRegion = vk::StridedDeviceAddressRegionKHR();
        buffer.MissRegion = vk::StridedDeviceAddressRegionKHR();
        buffer.HitGroupRegion = vk::StridedDeviceAddressRegionKHR();
        buffer.CallableRegion = vk::StridedDeviceAddressRegionKHR();
    }
} // namespace vr
