#pragma once

#include "Vulray/Buffer.h"

namespace vr
{


    enum class DescriptorBufferType : uint32_t
    {
        // These map straight to vulkan buffer usage flags

        Resource = (uint32_t)vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT,
        Sampler = (uint32_t)vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT,
        CombinedImageSampler = (uint32_t)(vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT)
    };

    struct DescriptorBuffer
    {
        AllocatedBuffer Buffer;
        // size of a single Descriptor set in the buffer
        // useful for offsetting into the buffer that has multiple IDENTICAL descriptor sets and binding one of them
        uint32_t SetCount = 0;
        uint32_t SingleDescriptorSize = 0;
        DescriptorBufferType Type = DescriptorBufferType::Resource;

        uint32_t GetOffsetToSet(uint32_t setIndex) const { return setIndex * SingleDescriptorSize; }
    };
    
    struct DescriptorItem
    {
        //constructor to initialize all fields
        DescriptorItem(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, uint32_t ArraySize, void* pItems, uint32_t dynamicArraySize = 0)
            : Type(type), Binding(binding), BindingOffset(0), ArraySize(ArraySize), StageFlags(stageFlags),
            pResources(reinterpret_cast<AllocatedBuffer*>(pItems)), // even if the item isn't a buffer, we can use this field since its a union and a 64-bit address
            DynamicArraySize(dynamicArraySize) 
        {
        }

        vk::DescriptorType Type;

        uint32_t Binding = 0;

        uint32_t BindingOffset = 0; // binding offset is filled in when creating the descriptor buffer

        uint32_t ArraySize = 0; // size of the binding array, if it is dynamic, this is the max size
        
        vk::ShaderStageFlags StageFlags = vk::ShaderStageFlagBits::eAll;

        // if this is non-zero, the array is dynamic and this is the size of the dynamic array
        // or in other words, the size of the array that p*** is pointing to
        uint32_t DynamicArraySize = 0; 

        // all of these are 64 bit pointers, so we can use a union
        union
        {
            // for normal buffers, eg uniform buffers, storage buffers
            // we only need the device address and size, so those fields must be set 
            AllocatedBuffer* pResources = nullptr;
            
            // for image samplers, combined image samplers
            AccessibleImage* pImages;

            // for acceleration structures
            vk::DeviceAddress* pAccelerationStructures;

            // for storage texel buffers
            AllocatedTexelBuffer* pTexelBuffers;
        };
        


        vk::DescriptorSetLayoutBinding GetLayoutBinding() const
        {
            return vk::DescriptorSetLayoutBinding()
                .setBinding(Binding)
                .setDescriptorType(Type)
                .setDescriptorCount(ArraySize)
                .setStageFlags(StageFlags);
        }

        // only one a
        vk::DeviceAddress GetAccelerationStructure( uint32_t resourceIndex = 0) const
        {
            return pAccelerationStructures[resourceIndex];
        }

        vk::DescriptorAddressInfoEXT GetTexelAddressinfo(uint32_t resourceIndex = 0) const
        {
            return vk::DescriptorAddressInfoEXT()
                .setRange(pTexelBuffers[resourceIndex].Buffer.Size)
                .setFormat(pTexelBuffers[resourceIndex].Format)
                .setAddress(pTexelBuffers[resourceIndex].Buffer.DevAddress);
        }

        vk::DescriptorAddressInfoEXT GetAddressInfo(uint32_t resourceIndex = 0) const
        {
            auto addressInfo = vk::DescriptorAddressInfoEXT()
                .setRange(pResources[resourceIndex].Size)
                .setFormat(vk::Format::eUndefined);

            switch (Type)
            {
            case vk::DescriptorType::eUniformBuffer:
                addressInfo.address = pResources[resourceIndex].DevAddress;
                break;
            case vk::DescriptorType::eStorageBuffer:
                addressInfo.address = pResources[resourceIndex].DevAddress;
                break;
            case vk::DescriptorType::eAccelerationStructureKHR:
                addressInfo.address = pResources[resourceIndex].DevAddress;
                break;
            case vk::DescriptorType::eStorageImage:
                addressInfo.address = pResources[resourceIndex].DevAddress;
                break;
            }

            return addressInfo;
        }

        vk::DescriptorImageInfo GetImageInfo(uint32_t resourceIndex = 0) const
        {
            return vk::DescriptorImageInfo()
                .setImageView(pImages[resourceIndex].View ? pImages[resourceIndex].View : vk::ImageView())
                .setSampler(pImages[resourceIndex].Sampler ? pImages[resourceIndex].Sampler : vk::Sampler())
                .setImageLayout(pImages[resourceIndex].Layout);
        }

    
        vk::DescriptorBufferInfo GetBufferInfo(uint32_t resourceIndex = 0) const
        {
            return vk::DescriptorBufferInfo()
                .setBuffer(pResources[resourceIndex].Buffer)
                .setRange(pResources[resourceIndex].Size);
        }
        
    };



} 
