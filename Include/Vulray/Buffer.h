#pragma once


namespace vr
{
    struct AllocatedBuffer
    {
        VmaAllocation Allocation = nullptr;
        vk::Buffer Buffer = nullptr;
        vk::DeviceAddress DevAddress = 0;

        uint64_t Size() const;
    };

    struct ImageAllocation
    {
        VmaAllocation Allocation = nullptr;
        vk::Image Image = nullptr;
    };

    uint32_t AlignUp(uint32_t value, uint32_t alignment);
    uint64_t AlignUp(uint64_t value, uint64_t alignment);
}