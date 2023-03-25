#include "Vulray/VulrayDevice.h"
#include "Vulray/Shader.h"
#include "Vulray/Descriptors.h"

namespace vr
{
    

    vk::DescriptorSet VulrayDevice::AllocateDescriptorSet(vk::DescriptorPool pool, vk::DescriptorSetLayout layout, uint32_t maxVariableDescriptors)
    {
        auto allocInfo = vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(pool)
            .setDescriptorSetCount(1)
            .setPSetLayouts(&layout);

        return mDevice.allocateDescriptorSets(allocInfo)[0]; // return the first set, because we only allocated one
    }

    void VulrayDevice::UpdateDescriptorSet(const std::vector<vk::WriteDescriptorSet>& writes)
    {
        mDevice.updateDescriptorSets(writes, {});
    }
    void VulrayDevice::UpdateDescriptorSet(const vk::WriteDescriptorSet& writes)
    {
        mDevice.updateDescriptorSets(1, &writes, 0, nullptr);
    }


    vk::DescriptorSetLayout VulrayDevice::CreateDescriptorSetLayout(const std::vector<DescriptorItem> &bindings)
    {
        bool hasDynamic = false;
        bool hasUpdateAfterBind = false;
        // prepare the layout bindings
        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(bindings.size()); 

        
        for (auto& binding : bindings)
        {
            layoutBindings.push_back(binding.GetLayoutBinding());
            if(binding.DynamicArray)
                hasDynamic = true;
            if(binding.mFlags & vk::DescriptorBindingFlagBits::eUpdateAfterBind)
                hasUpdateAfterBind = true;
        }

        //if there are dynamic bindings, we need to set the flags
        //the user is responsible for having the last binding in the set be a dynamic array

        //prepare the flags
        std::vector<vk::DescriptorBindingFlags> itemFlags;
        itemFlags.reserve(bindings.size());

        for(auto& binding : bindings)
        {
            itemFlags.push_back(binding.mFlags);
            if(binding.DynamicArray)
            {
                itemFlags.back() |= (vk::DescriptorBindingFlagBits::ePartiallyBound |
                vk::DescriptorBindingFlagBits::eVariableDescriptorCount);
            }
        }

        

        auto flags = vk::DescriptorSetLayoutBindingFlagsCreateInfo()
            .setBindingCount(static_cast<uint32_t>(itemFlags.size()))
            .setPBindingFlags(itemFlags.data());

        return mDevice.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
            .setBindingCount(static_cast<uint32_t>(layoutBindings.size()))
            .setFlags(hasUpdateAfterBind ? vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool : vk::DescriptorSetLayoutCreateFlags(0))
            .setPBindings(layoutBindings.data())
            .setPNext(&flags));
    }

    vk::DescriptorPool VulrayDevice::CreateDescriptorPool(const std::vector<vk::DescriptorPoolSize>& poolSizes, vk::DescriptorPoolCreateFlagBits flags, uint32_t maxSets)
    {
        return mDevice.createDescriptorPool(vk::DescriptorPoolCreateInfo()
            .setMaxSets(maxSets)
            .setFlags(flags)
            .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
            .setPPoolSizes(poolSizes.data()));
    }

    
    vk::PipelineLayout VulrayDevice::CreatePipelineLayout(const std::vector<vk::DescriptorSetLayout>& descLayouts)
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