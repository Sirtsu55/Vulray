#pragma once

namespace vr
{



    struct DescriptorItem
    {
        DescriptorItem() = default;
        DescriptorItem(uint32_t binding,
        vk::DescriptorType type, 
        vk::ShaderStageFlags stageFlags, 
        uint32_t descriptorCount = 1, 
        void* pitems = nullptr, 
        vk::DescriptorBindingFlagBits flags = (vk::DescriptorBindingFlagBits)0, bool dynamicArray = false)
            : Binding(binding), Type(type), StageFlags(stageFlags), DescriptorCount(descriptorCount),  pItems(pitems), mFlags(flags), DynamicArray(dynamicArray)
        {}

        vk::DescriptorSetLayoutBinding GetLayoutBinding() const
        {
            vk::DescriptorSetLayoutBinding binding;
            binding.setBinding(Binding);
            binding.setDescriptorCount(DescriptorCount);
            binding.setDescriptorType(Type);
            binding.setStageFlags(StageFlags);
            return binding;
        }

        vk::DescriptorBufferInfo GetBufferInfo(uint32_t descriptorIndex = 0) const
        {
            return vk::DescriptorBufferInfo()
                .setBuffer(reinterpret_cast<vk::Buffer*>(pItems)[descriptorIndex]) // index into array of buffers
                .setOffset(0)
                .setRange(VK_WHOLE_SIZE);
        }

        vk::DescriptorImageInfo GetImageInfo(uint32_t descriptorIndex = 0, vk::ImageLayout imgLayout = vk::ImageLayout::eGeneral) const
        {
            return vk::DescriptorImageInfo()
                .setImageLayout(imgLayout)
                .setImageView(reinterpret_cast<vk::ImageView*>(pItems)[descriptorIndex]); // index into array of image views
        }

        vk::WriteDescriptorSetAccelerationStructureKHR GetAccelerationStructureInfo(uint32_t descriptorIndex = 0, uint32_t count = 1)
        {
            return vk::WriteDescriptorSetAccelerationStructureKHR()
                .setAccelerationStructureCount(1) 
                .setPAccelerationStructures(reinterpret_cast<vk::AccelerationStructureKHR*>(pItems));
        }

        uint32_t Binding;
        vk::DescriptorType Type;
        vk::ShaderStageFlags StageFlags;
        uint32_t DescriptorCount = 1;

        // pointer to the actual item, eg. a vk::Image, vk::Buffer, etc.
        // if DescriptorCount > 1, this is a pointer to an array of items
        void* pItems = nullptr; 
        
        vk::DescriptorBindingFlagBits mFlags;

        bool DynamicArray = false;

    };

    struct DescriptorSet
    {
        DescriptorSet() = default;
        DescriptorSet(vk::DescriptorSet set, vk::DescriptorSetLayout layout, const std::vector<DescriptorItem>& items)
            : Set(set), Layout(layout), Items(std::move(items))
        {
        }

        // returns a write descriptor set for the given binding
        // user has to fill the type of descriptor and the actual item
        vk::WriteDescriptorSet GetWriteDescriptorSets(vk::DescriptorType type , uint32_t binding, uint32_t writeCount = 1, uint32_t writeStartIndex = 0) const
        {
            return vk::WriteDescriptorSet()
                .setDstSet(Set)
                .setDstBinding(binding)
                .setDescriptorType(type)
                .setDescriptorCount(writeCount)
                .setDstArrayElement(writeStartIndex);
        }

        vk::DescriptorSet Set;
        vk::DescriptorSetLayout Layout;
        std::vector<DescriptorItem> Items;
    };



} 
