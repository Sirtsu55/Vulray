#include "Vulray/VulrayDevice.h"
#include "Vulray/Shader.h"
#include "Vulray/Descriptors.h"

// These functions extract the addrss info / image info from the descriptor item
// and bind it to the corresponding pointer in the  DescriptorDataEXT struct
// they also return the size of the descriptor item
static void GetInfoOfDescriptorItem(const vr::DescriptorItem& item,
    uint32_t resourceIndex,
    vk::DescriptorAddressInfoEXT* pAddressInfo = nullptr,
    vk::DescriptorImageInfo* pImageInfo = nullptr,
    vk::Sampler* pSampler = nullptr,
    vk::DescriptorDataEXT* pData = nullptr);

static size_t GetDescriptorTypeDataSize(vk::DescriptorType type, const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& bufferProps);


namespace vr
{


    vk::DescriptorSetLayout VulrayDevice::CreateDescriptorSetLayout(const std::vector<DescriptorItem>& bindings)
    {

        bool hasDynamic = false;

        // prepare the layout bindings
        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(bindings.size()); 

        for (auto& binding : bindings)
        {
            layoutBindings.push_back(binding.GetLayoutBinding());
            if(binding.DynamicArraySize > 0)
                hasDynamic = true;
        }

        //if there are dynamic bindings, we need to set the flags
        //the user is responsible for having the last binding in the set be a dynamic array

        //prepare the flags
        std::vector<vk::DescriptorBindingFlags> itemFlags;
        if(hasDynamic)
        {
            itemFlags.reserve(bindings.size()); 
            for(auto& binding : bindings)
            {
                itemFlags.push_back((vk::DescriptorBindingFlagBits)0);
                if(binding.DynamicArraySize > 0)
                {
                    itemFlags.back() |= (vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount);
                }
            }
        }
        
        auto flags = vk::DescriptorSetLayoutBindingFlagsCreateInfo()
            .setBindingCount(static_cast<uint32_t>(itemFlags.size()))
            .setPBindingFlags(itemFlags.data());

        return mDevice.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
            .setBindingCount(static_cast<uint32_t>(layoutBindings.size()))
            .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT)
            .setPBindings(layoutBindings.data())
            .setPNext(hasDynamic ? &flags : nullptr));
    }

    void VulrayDevice::UpdateDescriptorBuffer(DescriptorBuffer& buffer,
        const DescriptorItem& item,
        uint32_t itemIndex,
        DescriptorBufferType type,
        uint32_t setIndexInBuffer,
        void* pMappedData)
    {
        // offset into the buffer
        uint32_t setOffset = buffer.GetOffsetToSet(setIndexInBuffer);

        char* mappedData = pMappedData == nullptr ? (char*)MapBuffer(buffer.Buffer) + setOffset : (char*)pMappedData + setOffset;
        
        char* cursor = mappedData + item.BindingOffset; // cursor to the item we want to update

        auto descGetInfo = vk::DescriptorGetInfoEXT()
            .setType(item.Type);

        auto addressInfo = vk::DescriptorAddressInfoEXT(); // in case of buffer

        auto imageInfo = vk::DescriptorImageInfo(); // in case of image or sampler

        vk::Sampler sampler = nullptr; // in case of sampler

        uint32_t dataSize = GetDescriptorTypeDataSize(item.Type, mDescriptorBufferProperties);

        GetInfoOfDescriptorItem(item, itemIndex, &addressInfo, &imageInfo, &sampler, &descGetInfo.data);

        mDevice.getDescriptorEXT(&descGetInfo, dataSize, cursor, mDynLoader); // write to cursor

        if(pMappedData == nullptr)
            UnmapBuffer(buffer.Buffer);
    }

    void VulrayDevice::UpdateDescriptorBuffer(DescriptorBuffer& buffer,
        const std::vector<DescriptorItem>& items,
        DescriptorBufferType type,
        uint32_t setIndexInBuffer,
        void* pMappedData)
    {
        // offset into the buffer
        uint32_t setOffset = buffer.GetOffsetToSet(setIndexInBuffer);

        // offset into the buffer and the right descriptor set
        char* mappedData = pMappedData == nullptr ? (char*)MapBuffer(buffer.Buffer) + setOffset : (char*)pMappedData + setOffset;
        
        char* cursor = mappedData; // cursor to the current item

        auto descGetInfo = vk::DescriptorGetInfoEXT();

        auto addressInfo = vk::DescriptorAddressInfoEXT(); // in case of buffer

        auto imageInfo = vk::DescriptorImageInfo(); // in case of image or sampler

        vk::Sampler sampler = nullptr; // in case of sampler

        for (uint32_t i = 0; i < items.size(); i++)
        {
            cursor = mappedData + items[i].BindingOffset; // move the cursor to the current item

            descGetInfo.type = items[i].Type; // same type for all items in the array
            size_t dataSize = GetDescriptorTypeDataSize(items[i].Type, mDescriptorBufferProperties);
            
            uint32_t arraySize = items[i].DynamicArraySize > 0 ? items[i].DynamicArraySize : items[i].ArraySize;

            for(uint32_t j = 0; j < arraySize; j++)
            {
                GetInfoOfDescriptorItem(items[i], j, &addressInfo, &imageInfo, &sampler, &descGetInfo.data);

                mDevice.getDescriptorEXT(&descGetInfo, dataSize, cursor, mDynLoader); // write to cursor

                cursor += dataSize;
            }
        }
        // we can unmap the buffer now, because we wrote all the data to it
        if(pMappedData == nullptr)
            UnmapBuffer(buffer.Buffer);
    }

    void VulrayDevice::UpdateDescriptorBuffer(DescriptorBuffer &buffer, const DescriptorItem &item, DescriptorBufferType type, uint32_t setIndexInBuffer, void *pMappedData)
    {
        char* mappedData = pMappedData == nullptr ? (char*)MapBuffer(buffer.Buffer) : (char*)pMappedData;
        
        uint32_t arraySize = item.DynamicArraySize > 0 ? item.DynamicArraySize : item.ArraySize;

        for(uint32_t i = 0; i < arraySize; i++)
        {
            // update the buffer for each item in the array, this is fast, because we don't have to map the buffer for each item
            UpdateDescriptorBuffer(buffer, item, i, type, setIndexInBuffer, mappedData);
        }

        if(pMappedData == nullptr)
            UnmapBuffer(buffer.Buffer);
    }

    void VulrayDevice::BindDescriptorBuffer(const std::vector<DescriptorBuffer> &buffers, vk::CommandBuffer cmdBuf)
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
            .setFlags(vk::PipelineLayoutCreateFlagBits::eIndependentSetsEXT)
            .setPSetLayouts(&descLayout));
    }

}


static void GetInfoOfDescriptorItem(const vr::DescriptorItem& item,
    uint32_t resourceIndex,
    vk::DescriptorAddressInfoEXT* pAddressInfo,
    vk::DescriptorImageInfo* pImageInfo,
    vk::Sampler* pSampler,
    vk::DescriptorDataEXT* pData)
{
    switch (item.Type)
    {

    // Resources

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

    // Images
    case vk::DescriptorType::eSampler:
        pSampler = item.GetSampler(resourceIndex);
        pData->pSampler = pSampler;
        break;
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
        return bufferProps.uniformBufferDescriptorSize;
    case vk::DescriptorType::eStorageBuffer:
        return bufferProps.storageBufferDescriptorSize;
    case vk::DescriptorType::eAccelerationStructureKHR:
        return bufferProps.accelerationStructureDescriptorSize;
    case vk::DescriptorType::eStorageTexelBuffer:
        return bufferProps.storageTexelBufferDescriptorSize;
    case vk::DescriptorType::eUniformTexelBuffer:
        return bufferProps.uniformTexelBufferDescriptorSize;
    case vk::DescriptorType::eStorageImage:
        return bufferProps.storageImageDescriptorSize;
    case vk::DescriptorType::eCombinedImageSampler:
        return bufferProps.combinedImageSamplerDescriptorSize;
    case vk::DescriptorType::eSampler:
        return bufferProps.samplerDescriptorSize;
    case vk::DescriptorType::eSampledImage:
        return bufferProps.sampledImageDescriptorSize;
    default:
        return 0;
    }
}