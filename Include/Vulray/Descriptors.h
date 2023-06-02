#pragma once

#include "Vulray/Buffer.h"

namespace vr
{

    /// @brief Enum that is used to specify the type of descriptors that will be stored in the buffer when calling CreateDescriptorBuffer(...)
    /// @note These enums map straight to the Vulkan BufferUsageFlagBits
    enum class DescriptorBufferType : uint32_t
    {
        /// @brief The buffer will store resource descriptors, eg uniform buffers, storage buffers
        Resource = (uint32_t)vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT,

        /// @brief The buffer will store image descriptors so sampled images
        Sampler = (uint32_t)vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT,

        /// @brief The buffer will store combined image samplers
        CombinedImageSampler = (uint32_t)(vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT)
    };

    struct DescriptorBuffer
    {
        /// @brief The buffer that will store the descriptors
        AllocatedBuffer Buffer;

        /// @brief Number of IDENTICAL descriptor sets in the buffer
        /// @note This is useful for offsetting into the buffer that has multiple IDENTICAL descriptor sets and binding one of them
        uint32_t SetCount = 0;

        /// @brief Size of a single descriptor in the buffer
        uint32_t SingleDescriptorSize = 0;

        /// @brief Type of descriptors that will be stored in the buffer, Default is Resource
        DescriptorBufferType Type = DescriptorBufferType::Resource;

        /// @brief If there are multiple descriptor sets in the buffer, this is the offset to the start of the set
        /// @param setIndex The index of the set to get the offset to
        /// @return The offset to the start of the set
        uint32_t GetOffsetToSet(uint32_t setIndex) const { return setIndex * SingleDescriptorSize; }
    };
    
    /// @brief Structure that defines a single descriptor item, such as a uniform buffer, storage buffer, image sampler, etc
    /// @note This supports having arrays of descriptors, such as an array of uniform buffers, or just a single descriptor
    struct DescriptorItem
    {
        /// @brief Default constructor
        /// @param binding The binding of the descriptor in the shader
        /// @param type The type of descriptor, eg uniform buffer, storage buffer, image sampler, etc
        /// @param stageFlags The shader stages that the descriptor will be used in
        /// @param ArraySize The size of the binding array, if the array is dynamic, this is the max size
        /// @param pItems Pointer to the items that will be stored in the descriptor, this can be a buffer, image, etc. Can be null and set later, but must be set before calling UpdateDescriptorSet(...)
        /// @param dynamicArraySize If this is non-zero, the array is dynamic and this is the size of the array that you want to be used. pItems must have dynamicArraySize many items
        DescriptorItem(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, uint32_t ArraySize, void* pItems, uint32_t dynamicArraySize = 0)
            : Type(type), Binding(binding), BindingOffset(0), ArraySize(ArraySize), StageFlags(stageFlags),
            pResources(reinterpret_cast<AllocatedBuffer*>(pItems)), // even if the item isn't a buffer, we can use this field since its a union and a 64-bit address
            DynamicArraySize(dynamicArraySize) 
        {
        }

        /// @brief Type of resource, eg uniform buffer, storage buffer, image sampler, etc
        vk::DescriptorType Type;

        /// @brief Binding of the descriptor in the shader
        uint32_t Binding = 0;

        /// @brief Offset of the binding in the descriptor set (filled in when creating the descriptor set)
        uint32_t BindingOffset = 0; 

        /// @brief Size of the binding array, if it is dynamic, this is the max size
        uint32_t ArraySize = 0;
        
        /// @brief Shader stages that the descriptor will be used in
        vk::ShaderStageFlags StageFlags = vk::ShaderStageFlagBits::eAll;

        /// @brief If this is non-zero, the descriptor is dynamic and specifies of how many items you want to update when calling UpdateDescriptorSet(...)
        /// @warning UpdateDescriptorSet(...) will throw a segfault if pItems is null or DynamicArraySize is bigger than the size of the pointer to an array of items
        uint32_t DynamicArraySize = 0;

        // all of these are 64 bit pointers, so we can use a union
        union
        {
            /// @brief Pointer to the resources that will be stored in the descriptor
            AllocatedBuffer* pResources = nullptr;
            
            /// @brief Pointer to the images that will be stored in the descriptor
            AccessibleImage* pImages;

            /// @brief Pointer to the acceleration structures that will be stored in the descriptor
            vk::DeviceAddress* pAccelerationStructures;

            /// @brief Pointer to the texel buffers that will be stored in the descriptor
            AllocatedTexelBuffer* pTexelBuffers;
        };
        

        /// @brief Gets the layout binding for the descriptor
        /// @return The layout binding for the descriptor
        vk::DescriptorSetLayoutBinding GetLayoutBinding() const
        {
            return vk::DescriptorSetLayoutBinding()
                .setBinding(Binding)
                .setDescriptorType(Type)
                .setDescriptorCount(ArraySize)
                .setStageFlags(StageFlags);
        }

        /// @brief Gets the device address of the acceleration structure
        /// @param resourceIndex The index of the resource to get the address of from the array of resources
        /// @return The device address of the acceleration structure
        /// @warning This will throw a segfault if the array is null or the index is out of bounds
        vk::DeviceAddress GetAccelerationStructure(uint32_t resourceIndex = 0) const
        {
            return pAccelerationStructures[resourceIndex];
        }

        /// @brief Gets the image view of the image
        /// @param resourceIndex The index of the resource to get the image view of from the array of resources
        /// @return The image view of the image
        vk::DescriptorAddressInfoEXT GetTexelAddressinfo(uint32_t resourceIndex = 0) const
        {
            return vk::DescriptorAddressInfoEXT()
                .setRange(pTexelBuffers[resourceIndex].Buffer.Size)
                .setFormat(pTexelBuffers[resourceIndex].Format)
                .setAddress(pTexelBuffers[resourceIndex].Buffer.DevAddress);
        }

        /// @brief Gets the AddressInfo of the resource
        /// @param resourceIndex The index of the resource to get the address info of from the array of resources
        /// @return The filled AddressInfo of the resource
        /// @warning This will throw a segfault if the array is null or the index is out of bounds
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
            case vk::DescriptorType::eStorageTexelBuffer:
                addressInfo.address = pTexelBuffers[resourceIndex].Buffer.DevAddress;
                break;
            case vk::DescriptorType::eUniformTexelBuffer:
                addressInfo.address = pTexelBuffers[resourceIndex].Buffer.DevAddress;
                break;
            }
            return addressInfo;
        }

        /// @brief Gets the image view of the image
        /// @param resourceIndex The index of the resource to get the image view of from the array of resources
        /// @return The image view of the image
        /// @warning This will throw a segfault if the array is null or the index is out of bounds
        vk::DescriptorImageInfo GetImageInfo(uint32_t resourceIndex = 0) const
        {
            return vk::DescriptorImageInfo()
                .setImageView(pImages[resourceIndex].View)
                .setImageLayout(pImages[resourceIndex].Layout);
        }

        /// @brief Gets the buffer info of the buffer
        /// @param resourceIndex The index of the resource to get the buffer info of from the array of resources
        /// @return The buffer info of the buffer
        /// @warning This will throw a segfault if the array is null or the index is out of bounds
        vk::DescriptorBufferInfo GetBufferInfo(uint32_t resourceIndex = 0) const
        {
            return vk::DescriptorBufferInfo()
                .setBuffer(pResources[resourceIndex].Buffer)
                .setRange(pResources[resourceIndex].Size);
        }
        
    };



} 
