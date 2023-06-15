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

            vk::Image Image;
            vk::ImageView View;
            vk::Sampler Sampler;

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
        
            DenoiserInterface(vr::VulrayDevice* device, uint32_t width, uint32_t height)
                : mDevice(device), mWidth(width), mHeight(height) {}

            virtual ~DenoiserInterface();

            // Delete unwanted constructors
            DenoiserInterface() = delete;
            DenoiserInterface(const DenoiserInterface&) = delete;

            virtual std::vector<Resource> GetRequiredResources() {}

            virtual const std::vector<Resource>& GetInputResources() const { return mInputResources; }

            virtual const std::vector<Resource>& GetOutputResources() const { return mOutputResources; }

        protected:
            
            // Reference to the vulray device that was used to create the denoiser
            // This is needed to destroy the resources
            vr::VulrayDevice* mDevice;

            uint32_t mWidth;
            uint32_t mHeight;

            std::vector<Resource> mInputResources;
            std::vector<Resource> mInternalResources;
            std::vector<Resource> mOutputResources;

            std::vector<AllocatedImage> mImages;
            
            void CreateResources(std::vector<Resource>& resources);

        };
    }

    // Type alias for the denoiser interface
    using Denoiser = std::unique_ptr<Denoise::DenoiserInterface>;
}