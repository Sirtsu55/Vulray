#pragma once


namespace vr
{
    struct AllocatedBuffer
    {
        VmaAllocation Allocation = nullptr;
        vk::Buffer Buffer = nullptr;
        vk::DeviceAddress DevAddress = 0;

        uint64_t Size = 0;
    };


    struct AllocatedTexelBuffer
    {
        AllocatedBuffer Buffer;
        vk::Format Format = vk::Format::eUndefined;
    };

    struct AllocatedImage
    {
        VmaAllocation Allocation = nullptr;
        vk::Image Image = nullptr;
        uint64_t Size = 0;
    };

    // an image that can be accessed by the GPU, eg a sampled image
    struct AccessibleImage
    {
        vk::ImageView View = nullptr;
        vk::ImageLayout Layout = vk::ImageLayout::eUndefined;
    };

    uint32_t AlignUp(uint32_t value, uint32_t alignment);
    uint64_t AlignUp(uint64_t value, uint64_t alignment);
}