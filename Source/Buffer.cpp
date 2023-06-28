#include "Vulray/Buffer.h"
#include "Vulray/VulrayDevice.h"


namespace vr
{

    AllocatedImage VulrayDevice::CreateImage(
    const vk::ImageCreateInfo& imgInfo,
    VmaAllocationCreateFlags flags)    
    {
        AllocatedImage outImage;
        VmaAllocationCreateInfo allocInf = {};
        allocInf.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInf.flags = flags;

        VmaAllocationInfo allocationInfo = {};

        auto result = (vk::Result)vmaCreateImage(mVMAllocator, (VkImageCreateInfo*)&imgInfo, &allocInf, (VkImage*)&outImage.Image, &outImage.Allocation, &allocationInfo);
        if(result != vk::Result::eSuccess)
        {
            VULRAY_FLOG_ERROR("Failed to create Image: %s", vk::to_string(result));
        }

        outImage.Size = allocationInfo.size;
        outImage.Width = imgInfo.extent.width;
        outImage.Height = imgInfo.extent.height;
        return outImage;
    }

    AllocatedBuffer VulrayDevice::CreateBuffer(
        vk::DeviceSize size,
        VmaAllocationCreateFlags flags,
        vk::BufferUsageFlags bufferUsage,
        uint32_t alignment)
    {
        AllocatedBuffer outBuffer = {};

        VmaAllocationCreateInfo allocInf = {};
        allocInf.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInf.flags = flags;

        vk::BufferCreateInfo bufInfo = {};
        bufInfo.setSize(size);
        bufInfo.setUsage(bufferUsage | vk::BufferUsageFlagBits::eShaderDeviceAddress);

        vk::Result result;

        if(alignment)
        {
            result = (vk::Result)vmaCreateBufferWithAlignment(mVMAllocator,
                (VkBufferCreateInfo*)&bufInfo, &allocInf,                 // type punning
                alignment,
                (VkBuffer*)&outBuffer.Buffer, &outBuffer.Allocation, nullptr);
        }
        else
        {
            result = (vk::Result)vmaCreateBuffer(mVMAllocator,
                (VkBufferCreateInfo*)&bufInfo, &allocInf,                   // type punning 
                (VkBuffer*)&outBuffer.Buffer, &outBuffer.Allocation, nullptr);
        }
        if(result != vk::Result::eSuccess)
        {
            VULRAY_FLOG_ERROR("Failed to create buffer: %s", vk::to_string(result));
            return outBuffer;
        }

        outBuffer.DevAddress = mDevice.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(outBuffer.Buffer));
        outBuffer.Size = size;
        return outBuffer;
    }

    AllocatedBuffer VulrayDevice::CreateInstanceBuffer(uint32_t instanceCount)
    {
        return CreateBuffer(
            instanceCount * sizeof(vk::AccelerationStructureInstanceKHR),
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
    }

    AllocatedBuffer VulrayDevice::CreateScratchBuffer(uint32_t size)
    {
        return CreateBuffer(size, 0,
            vk::BufferUsageFlagBits::eStorageBuffer,
            mAccelProperties.minAccelerationStructureScratchOffsetAlignment);
    }

    DescriptorBuffer VulrayDevice::CreateDescriptorBuffer(vk::DescriptorSetLayout layout,
                                                          std::vector<DescriptorItem> &items,
                                                          DescriptorBufferType type,
                                                          uint32_t setCount)
    {
        DescriptorBuffer outBuffer = {};

        uint32_t offset = 0;
        vk::BufferUsageFlags usageFlags = (vk::BufferUsageFlagBits)type; // DescriptorBufferType is a vulkan buffer usage flag enum

        vk::DeviceSize size = mDevice.getDescriptorSetLayoutSizeEXT(layout, mDynLoader);

        size = AlignUp(size, mDescriptorBufferProperties.descriptorBufferOffsetAlignment);

        // create a buffer that is big enough to hold all the descriptor sets and with the proper alignment
        outBuffer.Buffer = CreateBuffer(size * setCount, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, usageFlags, mDescriptorBufferProperties.descriptorBufferOffsetAlignment);

        // fill the offsets to the items
        for (auto& item : items)
        {
            uint32_t offset = mDevice.getDescriptorSetLayoutBindingOffsetEXT(layout, item.Binding, mDynLoader);
            item.BindingOffset = offset;
        }

        outBuffer.SetCount = setCount;
        outBuffer.SingleDescriptorSize = size;
        outBuffer.Type = type;

        return outBuffer;
    }

    void VulrayDevice::DestroyBuffer(AllocatedBuffer &buffer)
    {
        vmaDestroyBuffer(mVMAllocator, buffer.Buffer, buffer.Allocation);
        buffer.Buffer = nullptr;
        buffer.Allocation = nullptr;
        buffer.DevAddress = 0;
    }

    void VulrayDevice::DestroyImage(AllocatedImage& img)
    {
        vmaDestroyImage(mVMAllocator, img.Image, img.Allocation);
        img.Image = nullptr;
        img.Allocation = nullptr;
    }



    void VulrayDevice::UpdateBuffer(AllocatedBuffer alloc, void *data, const vk::DeviceSize size, uint32_t offset)
    {
        void* mappedData;
        vmaMapMemory(mVMAllocator, alloc.Allocation, &mappedData);
        memcpy((uint8_t*)mappedData + offset, data, size);
        vmaUnmapMemory(mVMAllocator, alloc.Allocation);
    }

    void VulrayDevice::CopyData(AllocatedBuffer src, AllocatedBuffer dst, vk::DeviceSize size, vk::CommandBuffer cmdBuf)
    {
        auto copyRegion = vk::BufferCopy()
            .setSize(size);
        cmdBuf.copyBuffer(src.Buffer, dst.Buffer, copyRegion);
    }

    void* VulrayDevice::MapBuffer(AllocatedBuffer &buffer)
    {
        void* mappedData;
        vmaMapMemory(mVMAllocator, buffer.Allocation, &mappedData);
        return mappedData;
    }

    void VulrayDevice::UnmapBuffer(AllocatedBuffer &buffer)
    {
        vmaUnmapMemory(mVMAllocator, buffer.Allocation);
    }

    void VulrayDevice::TransitionImageLayout(vk::Image image,
                                             vk::ImageLayout oldLayout,
                                             vk::ImageLayout newLayout,
                                             const vk::ImageSubresourceRange &range,
                                             vk::CommandBuffer cmdBuf,
                                             vk::PipelineStageFlags srcStage,
                                             vk::PipelineStageFlags dstStage)
    {
        //transition image layout with ..set functions
        auto barrier = vk::ImageMemoryBarrier()
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setImage(image)
            .setSubresourceRange(range);

        //set src and dst access masks
        switch (oldLayout)
        {
        case vk::ImageLayout::eUndefined:
            barrier.setSrcAccessMask((vk::AccessFlagBits)0);
            break;
        case vk::ImageLayout::ePreinitialized:
            barrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite);
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            barrier.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
            break;
        }
        //set dst access masks  
        switch (newLayout)
        {
        case vk::ImageLayout::eTransferDstOptimal:
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            barrier.setDstAccessMask(barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            if (barrier.srcAccessMask == (vk::AccessFlagBits)0)
                barrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            break;
        }
        cmdBuf.pipelineBarrier(
            srcStage,
            dstStage,
            (vk::DependencyFlagBits)0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        
    }

    uint32_t AlignUp(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    uint64_t AlignUp(uint64_t value, uint64_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

}