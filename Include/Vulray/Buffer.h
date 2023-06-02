#pragma once


namespace vr
{
    /// @brief Structure of a Buffer used for calls with Vulray
    struct AllocatedBuffer
    {
        /// @brief The allocation for the buffer, if this is null, the buffer is not allocated by Vulray
        /// @note If this is not null, the buffer is allocated by Vulray and should be freed by calling DestroyBuffer(...)
        /// If the buffer is allocated by the user, they do not need to set this field.
        VmaAllocation Allocation = nullptr;

        /// @brief The raw buffer handle
        vk::Buffer Buffer = nullptr;

        /// @brief The device address of the buffer
        vk::DeviceAddress DevAddress = 0;

        /// @brief The size of the buffer, without any alignment
        uint64_t Size = 0;
    };

    struct AllocatedTexelBuffer
    {
        /// @brief The allocation for the buffer
        AllocatedBuffer Buffer;

        /// @brief The format of the texel buffer
        vk::Format Format = vk::Format::eUndefined;
    };

    /// @brief Structure of an Image, to use with Vulray
    struct AllocatedImage
    {
        /// @brief The allocation for the image, if this is null, the image is not allocated by Vulray
        /// @note If this is not null, the image is allocated by Vulray and should be freed by calling DestroyImage(...)
        /// If the buffer is allocated by the user, they do not need to set this field.
        VmaAllocation Allocation = nullptr;
        
        /// @brief The raw image handle
        vk::Image Image = nullptr;
        
        uint32_t Width = 0;
        
        uint32_t Height = 0;

        uint64_t Size = 0;
    };

    /// @brief Structure of an Image, to use with Vulray
    /// @note Primarily used for images for DescriptorSets
    struct AccessibleImage
    {
        /// @brief The image view of the image
        vk::ImageView View = nullptr;

        /// @brief The image layout of the image
        vk::ImageLayout Layout = vk::ImageLayout::eUndefined;
    };

    /// @brief Aligns a value up to the specified alignment
    /// @param value The value to align
    /// @param alignment The alignment to align the value to
    /// @return The aligned value
    uint32_t AlignUp(uint32_t value, uint32_t alignment);

    /// @brief Aligns a value up to the specified alignment
    /// @param value The value to align
    /// @param alignment The alignment to align the value to
    /// @return The aligned value
    uint64_t AlignUp(uint64_t value, uint64_t alignment);
}