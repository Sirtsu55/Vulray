#include "Vulray/VulrayDevice.h"
#include "Vulray/Shader.h"
#include "Vulray/Descriptors.h"

// These functions extract the addrss info / image info from the descriptor item
// and bind it to the corresponding pointer in the  DescriptorDataEXT struct
// they also return the size of the descriptor item
static void GetAddressOfDescriptorItem(const vr::DescriptorItem& item,
    uint32_t resourceIndex,
    vk::DescriptorAddressInfoEXT* pAddressInfo = nullptr,
    vk::DescriptorDataEXT* pData = nullptr);

static void GetImageInfoOfDescriptorItem(const vr::DescriptorItem& item,
    uint32_t resourceIndex,
    vk::DescriptorImageInfo* pAddressInfo = nullptr,
    vk::DescriptorDataEXT* pData = nullptr);

static size_t GetDescriptorTypeDataSize(vk::DescriptorType type, const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& bufferProps);


namespace vr
{


    vk::DescriptorSetLayout VulrayDevice::CreateDescriptorSetLayout(const std::vector<DescriptorItem> &bindings)
    {
        // prepare the layout bindings
        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(bindings.size()); 

        
        for (auto& binding : bindings)
            layoutBindings.push_back(binding.GetLayoutBinding());

        return mDevice.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
            .setBindingCount(static_cast<uint32_t>(layoutBindings.size()))
            .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT)
            .setPBindings(layoutBindings.data()));
    }

    void VulrayDevice::UpdateDescriptorBuffer(DescriptorBuffer& buffer,
        const std::vector<DescriptorItem>& items,
        DescriptorBufferType type,
        uint32_t setIndexInBuffer)
    {
        // offset into the buffer
        uint32_t setOffset = buffer.GetOffsetToSet(setIndexInBuffer);

        auto bindingInfo = vk::DescriptorBufferBindingInfoEXT()
            .setAddress(buffer.Buffer.DevAddress + setOffset);

        char* mappedData = (char*)MapBuffer(buffer.Buffer) + setOffset;
        
        char* cursor = mappedData; // cursor to the current item

        auto descGetInfo = vk::DescriptorGetInfoEXT();

        auto addressInfo = vk::DescriptorAddressInfoEXT(); // in case of buffer

        auto imageInfo = vk::DescriptorImageInfo(); // in case of image or sampler


        for (uint32_t i = 0; i < items.size(); i++)
        {
            cursor = mappedData + items[i].BindingOffset; // move the cursor to the current item

            descGetInfo.type = items[i].Type; // same type for all items in the array
            size_t dataSize = GetDescriptorTypeDataSize(items[i].Type, mDescriptorBufferProperties);

            for(uint32_t j = 0; j < items[i].ArraySize; j++)
            {
                if(type == DescriptorBufferType::Resource)
                {
                    if(items[i].Type == vk::DescriptorType::eStorageImage) // storage images are resources, but they need image info
                        GetImageInfoOfDescriptorItem(items[i], j, &imageInfo, &descGetInfo.data);
                    else
                        GetAddressOfDescriptorItem(items[i], j, &addressInfo, &descGetInfo.data);

                    mDevice.getDescriptorEXT(&descGetInfo, dataSize, cursor, mDynLoader); // write to cursor

                    cursor += dataSize;
                }
                else if(type == DescriptorBufferType::Sampler | type == DescriptorBufferType::CombinedImageSampler)
                {
                    GetImageInfoOfDescriptorItem(items[i], j, &imageInfo, &descGetInfo.data);
                    
                    mDevice.getDescriptorEXT(&descGetInfo, dataSize, cursor, mDynLoader); // write to cursor

                    cursor += dataSize;
                }
            }
        }
        // we can unmap the buffer now, because we wrote all the data to it
        UnmapBuffer(buffer.Buffer);

    }

    std::vector<uint32_t> VulrayDevice::BindDescriptorBuffer(const std::vector<DescriptorBuffer>& buffers, vk::CommandBuffer cmdBuf)
    {
        std::vector<vk::DescriptorBufferBindingInfoEXT> bindingInfos;
        bindingInfos.reserve(buffers.size());

        std::vector<uint32_t> bufferIndices;
        bufferIndices.reserve(buffers.size());
        



        for(int i = 0; i < buffers.size(); i++)
        {
            bindingInfos.push_back(vk::DescriptorBufferBindingInfoEXT()
                .setAddress(buffers[i].Buffer.DevAddress)
                .setUsage((vk::BufferUsageFlagBits)buffers[i].Type));

            bufferIndices.push_back(i);
        }

        cmdBuf.bindDescriptorBuffersEXT(bindingInfos, mDynLoader);

        return bufferIndices;
    }

    void VulrayDevice::BindDescriptorSet(vk::PipelineLayout layout, uint32_t set, uint32_t bufferIndex, vk::DeviceSize offset, vk::CommandBuffer cmdBuf, vk::PipelineBindPoint bindPoint)
    {
        cmdBuf.setDescriptorBufferOffsetsEXT(bindPoint, layout, set, 1, &bufferIndex, &offset, mDynLoader);
    }

    void VulrayDevice::BindDescriptorSet(vk::PipelineLayout layout, uint32_t set, std::vector<uint32_t> bufferIndex, std::vector<vk::DeviceSize> offset, vk::CommandBuffer cmdBuf, vk::PipelineBindPoint bindPoint)
    {
        cmdBuf.setDescriptorBufferOffsetsEXT(bindPoint, layout, set, bufferIndex.size(), bufferIndex.data(), offset.data(), mDynLoader);
    }

    vk::PipelineLayout VulrayDevice::CreatePipelineLayout(const std::vector<vk::DescriptorSetLayout> &descLayouts)
    {
        //create pipeline layout
        return mDevice.createPipelineLayout(vk::PipelineLayoutCreateInfo()
            .setSetLayoutCount(static_cast<uint32_t>(descLayouts.size()))
            .setPSetLayouts(descLayouts.data()));
    }

    vk::PipelineLayout VulrayDevice::CreatePipelineLayout(vk::DescriptorSetLayout descLayout)
    {
        //create pipeline layout
        return mDevice.createPipelineLayout(vk::PipelineLayoutCreateInfo()
            .setSetLayoutCount(1)
            .setPSetLayouts(&descLayout));
    }

}


static void GetAddressOfDescriptorItem(const vr::DescriptorItem& item,
    uint32_t resourceIndex,
    vk::DescriptorAddressInfoEXT* pAddressInfo,
    vk::DescriptorDataEXT* pData)
{
    
    switch (item.Type)
    {
    case vk::DescriptorType::eUniformBuffer:
        *pAddressInfo = item.GetAddressInfo(resourceIndex);
        pData->pUniformBuffer = pAddressInfo;
        break;
    case vk::DescriptorType::eStorageBuffer:
        *pAddressInfo = item.GetAddressInfo(resourceIndex);
        pData->pStorageBuffer = pAddressInfo;
        break;
    case vk::DescriptorType::eAccelerationStructureKHR:
        pData->accelerationStructure = item.GetAccelerationStructure(resourceIndex);
        break;
    case vk::DescriptorType::eStorageTexelBuffer:
        *pAddressInfo = item.GetTexelAddressinfo(resourceIndex);
        pData->pStorageTexelBuffer = pAddressInfo;
        break;
    case vk::DescriptorType::eUniformTexelBuffer:
        *pAddressInfo = item.GetTexelAddressinfo(resourceIndex);
        pData->pUniformTexelBuffer = pAddressInfo;
        break;
        
    }
}

static void GetImageInfoOfDescriptorItem(const vr::DescriptorItem& item,
    uint32_t resourceIndex,
    vk::DescriptorImageInfo* pImageInfo,
    vk::DescriptorDataEXT* pData)
{
    switch (item.Type)
    {
    case vk::DescriptorType::eCombinedImageSampler:
        *pImageInfo = item.GetImageInfo(resourceIndex);
        pData->pCombinedImageSampler = pImageInfo;
        break;
    case vk::DescriptorType::eSampledImage:
        *pImageInfo = item.GetImageInfo(resourceIndex);
        pData->pSampledImage = pImageInfo;
        break;
    case vk::DescriptorType::eStorageImage:
        *pImageInfo = item.GetImageInfo(resourceIndex);
        pData->pStorageImage = pImageInfo;
        break;
    }
}

static size_t GetDescriptorTypeDataSize(vk::DescriptorType type, const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& bufferProps)
{
    switch (type)
    {
    case vk::DescriptorType::eUniformBuffer:
        return vr::AlignUp(bufferProps.uniformBufferDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    case vk::DescriptorType::eStorageBuffer:
        return vr::AlignUp(bufferProps.storageBufferDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    case vk::DescriptorType::eAccelerationStructureKHR:
        return vr::AlignUp(bufferProps.accelerationStructureDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    case vk::DescriptorType::eStorageTexelBuffer:
        return vr::AlignUp(bufferProps.storageTexelBufferDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    case vk::DescriptorType::eUniformTexelBuffer:
        return vr::AlignUp(bufferProps.uniformTexelBufferDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    case vk::DescriptorType::eStorageImage:
        return vr::AlignUp(bufferProps.storageImageDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    case vk::DescriptorType::eCombinedImageSampler:
        return vr::AlignUp(bufferProps.combinedImageSamplerDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    case vk::DescriptorType::eSampledImage:
        return vr::AlignUp(bufferProps.sampledImageDescriptorSize, bufferProps.descriptorBufferOffsetAlignment);
    default:
        return 0;
    }
}