#pragma once

#include "Vulray/Buffer.h"



namespace vr
{
    namespace Denoise
    {
        

        enum class ResourceType : uint32_t
        {
            GeneralInput,   // General input, so denoiser can be used in any context
            Output,         // Output  image
        };

        struct Resource
        {
            Resource() = default;
            AccessibleImage Image;

            // Getters
            ResourceType GetType() const { return Type; }
            vk::Format GetFormat() const { return Format; }

            // Setters
            void AddUsage(vk::ImageUsageFlagBits usage) { Usage |= usage; }

        private:
            vk::ImageUsageFlags Usage;
            ResourceType Type;
            vk::Format Format;

            // Allow the denoiser to access the private members
            friend class DenoiserInterface;
            friend class MedianDenoiser;

        };

        class DenoiserInterface
        {
        public:
        
            DenoiserInterface(vk::Device device, uint32_t height, uint32_t width)
                : mDevice(device), mHeight(height), mWidth(width) {}

            DenoiserInterface() = delete;
            DenoiserInterface(const DenoiserInterface&) = delete;

            virtual ~DenoiserInterface() {};

            /// @brief Initializes the denoiser with the supplied resources
            /// @param resources List of resources, MUST have the same type and number of resources as returned by GetRequiredResources(...)
            /// @note If Initialize(...) is called multiple times, the old resources will be destroyed and replaced with the new ones.
            /// @warning The resources MUST outlive the denoiser and MUST have the same dimensions as the denoiser.
            virtual void Initialize(std::vector<Resource>& resources) { if (!mResources.empty()) DestroyResources(); mResources = resources;}

            /// @brief Retrieves a list of resources that are required for the denoiser to work.
            /// User can just fill the required resource and call Initialize(...) with the list of resources.
            /// @return vr::Denoisers::Resource objects and the number of resources.
            virtual std::vector<Resource> GetRequiredResources() { return {}; }

            virtual void Denoise(vk::CommandBuffer cmdBuf) {};


            /// @brief Returns the resources that are used by the denoiser.
            /// @return List of resources that are used by the denoiser.
            /// @note This is a non-const function, so any changes to the returned list will be reflected in the denoiser.
            /// Useful if the user wants to change the resources that are used by the denoiser on the fly.
            [[nodiscard]] std::vector<Resource>& GetResources() { return mResources; }

            /// @brief Destroys the resources that are used by the denoiser.
            /// The resources can be destroyed manually by the user, but this function is provided for convenience.
            void DestroyResources() 
            { 
                for(auto& resource : mResources)
                {
                    mDevice.destroyImageView(resource.Image.View);
                    mDevice.destroySampler(resource.Image.Sampler);
                }
            }
        protected:
            
            vk::Device mDevice;
            uint32_t mHeight;
            uint32_t mWidth;
            std::vector<Resource> mResources;
        };
    }

    // Type alias for the denoiser interface
    using Denoiser = std::unique_ptr<Denoise::DenoiserInterface>;
}