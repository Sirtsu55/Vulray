#pragma once
#include "Vulray/Shader.h"
#include "Vulray/SBT.h"
#include "Vulray/AccelStruct.h"
#include "Vulray/Descriptors.h"


namespace vr
{

    class VulrayDevice
    {
    public:
        VulrayDevice(vk::Instance inst, vk::Device dev, vk::PhysicalDevice physDev);
        ~VulrayDevice();


        vk::Device GetDevice() const { return mDevice; }

        vk::PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

        vk::Instance GetInstance() const { return mInstance; }

        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingProperties() const { return mRayTracingProperties; }

        vk::PhysicalDeviceAccelerationStructurePropertiesKHR GetAccelerationStructureFeatures() const { return mAccelProperties; }

        //--------------------------------------------------------------------------------
        // Command Buffer functions
        //--------------------------------------------------------------------------------
        [[nodiscard]] vk::CommandBuffer CreateCommandBuffer(vk::CommandPool pool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

        [[nodiscard]] std::vector<vk::CommandBuffer> CreateCommandBuffers(vk::CommandPool pool, uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

        //Image layout transition
        void TransitionImageLayout(vk::Image image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout, 
            const vk::ImageSubresourceRange& range,
            vk::CommandBuffer cmdBuf,
            vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eAllCommands);

        //--------------------------------------------------------------------------------
        //Acceleration Structure functions
        //--------------------------------------------------------------------------------

        //Creates a bottom level acceleration structure and gives BuildInfo for the build
        [[nodiscard]] std::pair<BLASHandle, BLASBuildInfo> CreateBLAS(const BLASCreateInfo& info);

        // Builds the acceleration structure and returns the scratch buffer for building, the scratch buffer is not destroyed
        // After the build, the scratch buffer should be destroyed by the user
        [[nodiscard]] AllocatedBuffer BuildBLAS(std::vector<BLASBuildInfo>& buildInfos, vk::CommandBuffer cmdBuf);


        //Creates a top level acceleration structure and gives BuildInfo for the build
        [[nodiscard]] std::pair<TLASHandle, TLASBuildInfo> CreateTLAS(const TLASCreateInfo& info);

        // Builds the acceleration structure 
        [[nodiscard]] void BuildTLAS(TLASBuildInfo& buildInfos, vk::CommandBuffer cmdBuf);

        //Adds barrier to the command buffer to ensure the acceleration structure is built before other acceleration structures are built
        void AddAccelerationBuildBarrier(vk::CommandBuffer cmdBuf);


        void DestroyBLAS(BLASHandle& blas);


        //--------------------------------------------------------------------------------
        //Buffer functions
        //--------------------------------------------------------------------------------

        [[nodiscard]] ImageAllocation CreateImage(
            const vk::ImageCreateInfo& imgInfo,
            VmaAllocationCreateFlags flags);

        [[nodiscard]] AllocatedBuffer CreateBuffer(
            vk::DeviceSize size,
            VmaAllocationCreateFlags flags,
            vk::BufferUsageFlags bufferUsage,
            uint32_t alignment = 0);

        //Uploads data to a buffer, via mapping the buffer and memcpy
        void UpdateBuffer(AllocatedBuffer alloc, void* data, const vk::DeviceSize size, uint32_t offset = 0);

        //Copy Data from a buffer to another buffer
		void CopyData(AllocatedBuffer src, AllocatedBuffer dst, vk::DeviceSize size, vk::CommandBuffer cmdBuf);

        //Mapping
        void* MapBuffer(AllocatedBuffer &buffer);

        void UnmapBuffer(AllocatedBuffer &buffer);
        
        void DestroyBuffer(AllocatedBuffer& buffer);

        void DestroyImage(ImageAllocation & img);




        //--------------------------------------------------------------------------------
        //Shader functions
        //--------------------------------------------------------------------------------

        [[nodiscard]] Shader CreateShaderFromSPV(ShaderCreateInfo& info);

        [[nodiscard]] vk::ShaderModule CreateShaderModule(const std::vector<uint32_t>& spvCode);

        void DestroyShader(Shader & shader);

        //--------------------------------------------------------------------------------
        //Pipeline functions
        //--------------------------------------------------------------------------------

        [[nodiscard]] vk::PipelineLayout CreatePipelineLayout(vk::DescriptorSetLayout descLayout);

        [[nodiscard]] vk::PipelineLayout CreatePipelineLayout(const std::vector<vk::DescriptorSetLayout>& descLayouts);

        [[nodiscard]] vk::Pipeline CreateRayTracingPipeline(vk::PipelineLayout layout, const ShaderBindingTable& info, uint32_t recursuionDepth);

        //--------------------------------------------------------------------------------
        //Descriptor functions
        //--------------------------------------------------------------------------------

        [[nodiscard]] vk::DescriptorSetLayout CreateDescriptorSetLayout(const std::vector<DescriptorItem> &bindings);

        [[nodiscard]] vk::DescriptorPool CreateDescriptorPool(const std::vector<vk::DescriptorPoolSize>& poolSizes, vk::DescriptorPoolCreateFlagBits flags, uint32_t maxSets);

        //if maxVariableDescriptors is 0, it will create a descriptor set without variable descriptors
        [[nodiscard]] vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorPool pool, vk::DescriptorSetLayout layout, uint32_t maxVariableDescriptors = 0);

        void UpdateDescriptorSet(const std::vector<vk::WriteDescriptorSet>& writes);

        void UpdateDescriptorSet(const vk::WriteDescriptorSet & writes);

        //--------------------------------------------------------------------------------
        //SBT functions
        //--------------------------------------------------------------------------------

        [[nodiscard]] std::vector<uint8_t> GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup, uint32_t groupCount);

        //Writes data to a shader record in the SBT buffer, if mappedData is not null, it will be used instead of mapping the buffer
        void WriteToSBT(SBTBuffer sbtBuf, ShaderGroup group, uint32_t groupIndex, void* data, uint32_t dataSize, void* mappedData = nullptr);


        //Creates a buffer for the SBT with the correct alignment and makes room for all shader records.
        //Copies vulkan SBT opaque handles to the buffer to avaid boilerplate code and make it easier to use
        //The buffer is ready to filled with the shader records
        [[nodiscard]] SBTBuffer CreateSBT(vk::Pipeline pipeline, const ShaderBindingTable& sbt);


        void DestroySBTBuffer(SBTBuffer& buffer);




        //--------------------------------------------------------------------------------
        //Dispatch Functions
        //--------------------------------------------------------------------------------

        void DispatchRays(vk::CommandBuffer cmdBuf, const vk::Pipeline rtPipeline, const SBTBuffer& buffer, uint32_t width, uint32_t height, uint32_t depth = 1);

    private:


        vk::DispatchLoaderDynamic mDynLoader;

        vk::Instance mInstance;
        vk::Device mDevice;
        vk::PhysicalDevice mPhysicalDevice;
        
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR mRayTracingProperties;
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR mAccelProperties;

        VmaAllocator mVMAllocator;

    };

    vk::AccelerationStructureGeometryDataKHR ConvertToVulkanGeometry(const GeometryData& geom);
    
} 

