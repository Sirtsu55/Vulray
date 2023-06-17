#pragma once

#include "Vulray/Buffer.h"



namespace vr
{
    class VulrayDevice;
    namespace Denoise
    {
        enum class ResourceType : uint32_t
        {
            // Inputs have first bit set to 1
            // Outputs have second bit set to 1
            
            InputGeneral    = 0b00000001,   // General input, so denoiser can be used in any context

            OutputFinal     = 0b00000010,   // Output  image
        };

        struct Resource
        {
            Resource() = default;

            AllocatedImage AllocImage;
            AccessibleImage AccessImage;

            // Getters
            ResourceType GetType() const { return Type; }
            vk::Format GetFormat() const { return Format; }

            // Setters
            void AddUsage(vk::ImageUsageFlagBits usage) { Usage |= usage; }

        private:
            vk::ImageUsageFlags Usage;
            ResourceType Type;
            vk::Format Format;

            // Allow the denoisers to access the private members
            friend class DenoiserInterface;
            friend class GaussianBlurDenoiser;
        };

        class DenoiserInterface
        {
        public:
        
            DenoiserInterface(vr::VulrayDevice* device, uint32_t width, uint32_t height)
                : mDevice(device), mWidth(width), mHeight(height) {}

            virtual ~DenoiserInterface();

            // Delete unwanted constructors
            DenoiserInterface() = delete;
            DenoiserInterface(const DenoiserInterface&) = delete;

            /// @brief Initialize the denoiser
            /// @param inputUsage The additional image usage flags for the input image
            /// @param outputUsage The additional image usage flags for the output image
            /// @note The internal usage flags will be added automatically
            virtual void Initialize(vk::ImageUsageFlags inputUsage = (vk::ImageUsageFlags)0,
                vk::ImageUsageFlags outputUsage = (vk::ImageUsageFlags)0) {};

            // TODO: Let the user allocate the resources and pass them to the denoiser

            /// @brief Get the resources that are required for the denoiser
            /// @return The required resources
            /// @note The resources vector doesn't contain allocated images, only the required information to allocate them
            virtual std::vector<Resource> GetRequiredResources() { return std::vector<Resource>(); }

            /// @brief Get the input resources that are used by the denoiser
            /// @return The input resources
            virtual const std::vector<Resource>& GetInputResources() const { return mInputResources; }

            /// @brief Get the output resources that are used by the denoiser
            /// @return The internal resources
            virtual const std::vector<Resource>& GetOutputResources() const { return mOutputResources; }


            /// @brief Denoise the image
            /// @param cmdBuffer The command buffer to use for the denoising, must be in recording state and this function uses push constants for the settings
            virtual void Denoise(vk::CommandBuffer cmdBuffer) {};

            /// @brief Get the settings struct
            /// @tparam T The type of the settings struct -> DenoiserX::Settings
            /// @return The settings struct
            /// @warning If a settings struct from another denoiser is requested, it can cause in segmentation faults, because the memory layout is different
            template<typename T>
            T GetDenoiserSettings() const { return *(T*)mSettings; }

            /// @brief Set the settings struct
            /// @tparam T The type of the settings struct -> DenoiserX::Settings
            /// @param settings The settings struct
            /// @warning If a settings struct from another denoiser is set, it can cause in segmentation faults, because the memory layout is different
            template<typename T>
            void SetDenoiserSettings(const T& settings) { *(T*)mSettings = settings; }

        protected:
            
            // Reference to the vulray device that was used to create the denoiser
            // This is needed to destroy the resources
            vr::VulrayDevice* mDevice;

            uint32_t mWidth;
            uint32_t mHeight;

            std::vector<Resource> mInputResources;
            std::vector<Resource> mInternalResources;
            std::vector<Resource> mOutputResources;

            // pointer to the settings struct, allocated by a derived class
            void* mSettings = nullptr;

            void CreateResources(std::vector<Resource>& resources, vk::ImageUsageFlags inputUsage, vk::ImageUsageFlags outputUsage);

        };
    }

    // Type alias for the denoiser interface
    using Denoiser = std::unique_ptr<Denoise::DenoiserInterface>;
}