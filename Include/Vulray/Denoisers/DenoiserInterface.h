#pragma once

#include "Vulray/Buffer.h"

namespace vr
{
    class VulrayDevice;
    namespace Denoise
    {
        enum class ResourceType : uint32_t
        {
            Input = 0b01,
            Output = 0b10,
            Internal = 0b11,

            /// @brief Input resource that can be anything
            InputGeneral = Input | 0b100,

            /// @brief Output resource that is the final image
            OutputFinal = Output | 0b100,

        };

        struct Resource
        {
            Resource() = default;

            AllocatedImage AllocImage;
            AccessibleImage AccessImage;

            vk::ImageUsageFlags Usage;
            ResourceType Type;
            vk::Format Format;
        };

        struct DenoiserSettings
        {
            uint32_t Width;
            uint32_t Height;

            vk::ImageUsageFlags InputUsage;
            vk::ImageUsageFlags OutputUsage;
        };

        class DenoiserInterface
        {
          public:
            DenoiserInterface(vr::VulrayDevice* device, const DenoiserSettings& settings)
                : mDevice(device), mSettings(settings)
            {
            }

            virtual ~DenoiserInterface();

            // Delete unwanted constructors
            DenoiserInterface() = delete;
            DenoiserInterface(const DenoiserInterface&) = delete;

            // TODO: Let the user allocate the resources and pass them to the denoiser

            /// @brief Get the resources that are required for the denoiser
            /// @return The required resources
            /// @note The resources vector doesn't contain allocated images, only the required information to allocate
            /// them
            virtual std::vector<Resource> GetRequiredResources() { return std::vector<Resource>(); }

            /// @brief Get the input resources that are used by the denoiser
            /// @return The input resources
            virtual const std::vector<Resource>& GetInputResources() const { return mInputResources; }

            /// @brief Get the output resources that are used by the denoiser
            /// @return The internal resources
            virtual const std::vector<Resource>& GetOutputResources() const { return mOutputResources; }

            /// @brief Denoise the image
            /// @param cmdBuffer The command buffer to use for the denoising, must be in recording state and this
            /// function uses push constants for the settings
            virtual void Denoise(vk::CommandBuffer cmdBuffer) {};

            /// @brief Get the Parameters struct
            /// @tparam T The type of the settings struct -> DenoiserX::Parameters
            /// @return The Parameters struct
            /// @warning If a settings struct from another denoiser is requested, it can cause in segmentation faults,
            /// because the memory layout is different
            template <typename T> T GetDenoiserParams() const { return *(T*)mDenoiserParams; }

            /// @brief Set the Parameters struct
            /// @tparam T The type of the Parameters struct -> DenoiserX::Parameters
            /// @param params The Parameters struct
            /// @warning If a Parameters struct from another denoiser is set, it can cause in segmentation faults,
            /// because the memory layout is different
            template <typename T> void SetDenoiserParams(const T& params) { *(T*)mDenoiserParams = params; }

          protected:
            // Reference to the vulray device that was used to create the denoiser
            // This is needed to destroy the resources
            vr::VulrayDevice* mDevice;

            DenoiserSettings mSettings;

            std::vector<Resource> mInputResources;
            std::vector<Resource> mInternalResources;
            std::vector<Resource> mOutputResources;

            // pointer to the settings struct, allocated by a derived class
            void* mDenoiserParams = nullptr;

            void CreateResources(std::vector<Resource>& resources, vk::ImageUsageFlags inputUsage,
                                 vk::ImageUsageFlags outputUsage);
        };
    } // namespace Denoise

    // Type alias for the denoiser interface
    using Denoiser = std::unique_ptr<Denoise::DenoiserInterface>;
} // namespace vr