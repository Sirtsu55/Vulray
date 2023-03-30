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
        return handles;
    }


    void VulrayDevice::WriteToSBT(SBTBuffer sbtBuf, ShaderGroup group, uint32_t groupIndex, void* data, uint32_t dataSize, void* mappedData)
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

    SBTBuffer VulrayDevice::CreateSBT(vk::Pipeline pipeline, const ShaderBindingTable& sbt)
    {
        assert(sbt.RayGenShader.Module && "Ray gen shader must be set");
        SBTBuffer outSBT;

        uint32_t numShaderGroups = static_cast<uint32_t>(1 + sbt.MissShaders.size() + sbt.HitGroups.size() + sbt.CallableShaders.size());

        uint32_t handleSize = mRayTracingProperties.shaderGroupHandleSize;
        uint32_t handleSizeAligned = AlignUp(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);

        uint32_t rgenSize = AlignUp(sbt.RayGenShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t missSize = AlignUp(sbt.MissShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t hitSize = AlignUp(sbt.HitShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);
        uint32_t callSize = AlignUp(sbt.CallableShaderRecordSize + mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment);


        //don't add size if there are no shaders of that type, NOTE: raygen shader is required so we don't check for that
        if (sbt.MissShaders.size() == 0)
            missSize = 0;
        if (sbt.HitGroups.size() == 0)
            hitSize = 0;
        if (sbt.CallableShaders.size() == 0)
            callSize = 0;



        //Create all buffers for the SBT
        outSBT.RayGenBuffer = CreateBuffer(rgenSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR);
        if(missSize > 0)
            outSBT.MissBuffer = CreateBuffer(missSize * sbt.MissShaders.size(), VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR);
        if(hitSize > 0)
            outSBT.HitGroupBuffer = CreateBuffer(hitSize * sbt.HitGroups.size(), VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR);
        if(callSize > 0)
            outSBT.CallableBuffer = CreateBuffer(callSize * sbt.CallableShaders.size(), VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR);
        
        //map all the buffers


        // fill in offsets for all shader groups
        
        outSBT.RayGenRegion = vk::StridedDeviceAddressRegionKHR()
            .setDeviceAddress(outSBT.RayGenBuffer.DevAddress)
            .setStride(rgenSize)
            .setSize(rgenSize); // only one ray gen shader
        if(sbt.MissShaders.size() > 0)
            outSBT.MissRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.MissBuffer.DevAddress) // only one ray gen shader
                .setStride(missSize)
                .setSize(missSize * sbt.MissShaders.size());
        if(sbt.HitGroups.size() > 0)
            outSBT.HitGroupRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.HitGroupBuffer.DevAddress)
                .setStride(hitSize)
                .setSize(hitSize * sbt.HitGroups.size());
        if(sbt.CallableShaders.size() > 0)
            outSBT.CallableRegion = vk::StridedDeviceAddressRegionKHR()
                .setDeviceAddress(outSBT.CallableBuffer.DevAddress)
                .setStride(callSize)
                .setSize(callSize * sbt.CallableShaders.size());


        uint32_t dataOffset = 0; //offset into the SBT buffer
        uint32_t handleOffset = 0; // opaque handle offset
        
        std::vector<uint8_t> sbtHandles = GetHandlesForSBTBuffer(pipeline, 0, numShaderGroups);
        
        //copy records and shader handles into the SBT buffer
        void* rgenData = MapBuffer(outSBT.RayGenBuffer);
        {
            if(rgenData)
                memcpy(rgenData, sbtHandles.data() + handleOffset, mRayTracingProperties.shaderGroupHandleSize);
            handleOffset += mRayTracingProperties.shaderGroupHandleSize; // offset in the opaque handle vector
        }
        void* missData = missSize > 0 ? MapBuffer(outSBT.MissBuffer) : nullptr;
        for(uint32_t i = 0; i < sbt.MissShaders.size(); i++)
        {
            if(missData)
                memcpy(missData, sbtHandles.data() + handleOffset, mRayTracingProperties.shaderGroupHandleSize);
            handleOffset += mRayTracingProperties.shaderGroupHandleSize;
        }
        void* hitData = hitSize > 0 ? MapBuffer(outSBT.HitGroupBuffer) : nullptr;
        for(uint32_t i = 0; i < sbt.HitGroups.size(); i++)
        {
            if(hitData)
                memcpy(hitData, sbtHandles.data() + handleOffset, mRayTracingProperties.shaderGroupHandleSize);
            handleOffset += mRayTracingProperties.shaderGroupHandleSize;
        }
        
        void* callData = callSize > 0 ? MapBuffer(outSBT.CallableBuffer) : nullptr;
        for(uint32_t i = 0; i < sbt.CallableShaders.size(); i++)
        {
            if(callData)
                memcpy(callData, sbtHandles.data() + handleOffset, mRayTracingProperties.shaderGroupHandleSize);
            handleOffset += mRayTracingProperties.shaderGroupHandleSize;
        }

        //unmap all the buffers if they were mapped
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