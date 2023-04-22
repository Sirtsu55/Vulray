#pragma once
#include "Vulray/Shader.h"
#include "Vulray/SBT.h"
#include "Vulray/AccelStruct.h"
#include "Vulray/Descriptors.h"


namespace vr
{

    // Logging types
    enum class MessageType { Verbose, Info, Warning, Error};

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

        vk::PhysicalDeviceDescriptorBufferPropertiesEXT GetDescriptorBufferProperties() const { return mDescriptorBufferProperties; } 

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

        // Records the acceleration structure build to the command buffer
        // it is assumed that the scratch buffer address is set in the build infos
        // To assure that the scratch buffer is big enough, the user should call CreateScratchBufferBLAS(...)
        void BuildBLAS(const std::vector<BLASBuildInfo> &buildInfos, vk::CommandBuffer cmdBuf);

        // Updates the acceleration structure and returns the scratch buffer for building
        // After the update, user should call BuildBLAS with the returned build infos
        [[nodiscard]] BLASBuildInfo UpdateBLAS(BLASUpdateInfo& updateInfo);

        // Creates a SINGLE scratch buffer for building acceleration structures
        // Also sets the scratch device address for the build infos
        // If you want to allocate separate scratch buffer for many BLAS, you need to call this function for each BLAS
        // If the BLAS is updated regularly, it is recommended to create a separate scratch buffer for the updating BLAS
        // and use the scratch buffer for the updating
        // if there are many Dynamic BLASes, it is recommended to create a big scratch buffer for all of them
        // which are separate from static BLASes
        [[nodiscard]] AllocatedBuffer CreateScratchBufferBLAS(std::vector<BLASBuildInfo>& buildInfos, vk::BuildAccelerationStructureModeKHR mode = vk::BuildAccelerationStructureModeKHR::eBuild);

        // Same as above, but for a single BLAS
        [[nodiscard]] AllocatedBuffer CreateScratchBufferBLAS(BLASBuildInfo& buildInfo, vk::BuildAccelerationStructureModeKHR mode = vk::BuildAccelerationStructureModeKHR::eBuild);


        [[nodiscard]] void BindScratchBufferToBuildInfo(vk::DeviceSize scratchBuffer, BLASBuildInfo& buildInfo);

        [[nodiscard]] void BindScratchBufferToBuildInfo(vk::DeviceSize scratchBuffer, TLASBuildInfo& buildInfo);

        // Creates a SINGLE scratch buffer for building acceleration structure, and set scratch address for the build info
        // Only one Scratch buffer is created for the TLAS
        // If you want to allocate scratch buffer for many TLAS, you need to call this function for each TLAS
        // One large scratch buffer is not created because, there are only few TLASes in a scene
        [[nodiscard]] AllocatedBuffer CreateScratchBufferTLAS(TLASBuildInfo& buildInfo);

        //Creates a top level acceleration structure and gives BuildInfo for the build
        [[nodiscard]] std::pair<TLASHandle, TLASBuildInfo> CreateTLAS(const TLASCreateInfo& info);

        // Builds the acceleration structure 
        void BuildTLAS(TLASBuildInfo& buildInfo, 
            const AllocatedBuffer& InstanceBuffer, uint32_t instanceCount, 
            vk::CommandBuffer cmdBuf);

        [[nodiscard]] std::pair<TLASHandle, TLASBuildInfo> UpdateTLAS(TLASHandle& oldTLAS, TLASBuildInfo& oldBuildInfo, bool destroyOld = true);

        //Adds barrier to the command buffer to ensure the acceleration structure is built before other acceleration structures are built
        void AddAccelerationBuildBarrier(vk::CommandBuffer cmdBuf);

        void DestroyBLAS(BLASHandle& blas);

        void DestroyTLAS(TLASHandle& tlas);

        void DestroyAccelerationStructure(const vk::AccelerationStructureKHR& accel);


        //--------------------------------------------------------------------------------
        //Buffer functions
        //--------------------------------------------------------------------------------

        [[nodiscard]] AllocatedImage CreateImage(
            const vk::ImageCreateInfo& imgInfo,
            VmaAllocationCreateFlags flags);

        [[nodiscard]] AllocatedBuffer CreateBuffer(
            vk::DeviceSize size,
            VmaAllocationCreateFlags flags,
            vk::BufferUsageFlags bufferUsage,
            uint32_t alignment = 0);

        [[nodiscard]] AllocatedBuffer CreateInstanceBuffer(uint32_t instanceCount);

        // Creates a buffer for storing the descriptor sets
        // Furthermore it also gives a binding offset for each descriptor item
        // setCount is the number of descriptor sets that will fit in the buffer
        // Conditions(not checked by Vulray):
        // 1. ALl of the descriptor items in the layout should be of the same type
        [[nodiscard]] DescriptorBuffer CreateDescriptorBuffer(vk::DescriptorSetLayout layout,
            std::vector<DescriptorItem>& items,
            DescriptorBufferType type,
            uint32_t setCount = 1);

        //Uploads data to a buffer, via mapping the buffer and memcpy
        void UpdateBuffer(AllocatedBuffer alloc, void* data, const vk::DeviceSize size, uint32_t offset = 0);

        //Copy Data from a buffer to another buffer
		void CopyData(AllocatedBuffer src, AllocatedBuffer dst, vk::DeviceSize size, vk::CommandBuffer cmdBuf);

        //Mapping
        void* MapBuffer(AllocatedBuffer &buffer);

        void UnmapBuffer(AllocatedBuffer &buffer);
        
        void DestroyBuffer(AllocatedBuffer& buffer);

        void DestroyImage(AllocatedImage & img);




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

        // Updates the descriptor buffer with the descriptor items in the set
        // Conditions(not checked by Vulray):
        // 1. For each element in the items vector, the buffer will be updated with the given data in DescriptorItem::pImageViews/pResources pointer
        // 2. the pointers should point to an array of DescriptorItem::count elements
        // 3. If there are n descriptor sets in the buffer, the setIndexInBuffer should be used to specify the nth set to update
        // 4. If there are multiple descriptor sets in the buffer, they MUST have the same layout
        // mappedData is the pointer to the mapped data of the buffer, if it is null, the buffer will be mapped and unmapped

        void UpdateDescriptorBuffer(DescriptorBuffer& buffer,
            const std::vector<DescriptorItem>& items,
            DescriptorBufferType type,
            uint32_t setIndexInBuffer = 0,
            void* pMappedData = nullptr);

        // Updates the descriptor buffer with the single descriptor item including all the elements in the array
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer,
            const DescriptorItem& item,
            DescriptorBufferType type,
            uint32_t setIndexInBuffer = 0,
            void* pMappedData = nullptr);

        // Updates the descriptor buffer with one element of the descriptor item 
        // Conditions are the same as the other UpdateDescriptorBuffer function
        // itemIndex is the index of the descriptor item in the layout, so DescriptorItem::p***[itemIndex] has to be valid
        // mappedData is the pointer to the mapped data of the buffer, if it is null, the buffer will be mapped and unmapped
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer,
            const DescriptorItem& item,
            uint32_t itemIndex,
            DescriptorBufferType type,
            uint32_t setIndexInBuffer = 0,
            void* pMappedData = nullptr);

        // Binds the descriptor buffer to the command buffer
        // returns the buffer indices that were bound
        // the buffer indices are used to offet the buffer to the right descriptor set 
        void BindDescriptorBuffer(const std::vector<DescriptorBuffer>& buffers, vk::CommandBuffer cmdBuf);

        void BindDescriptorSet(
            vk::PipelineLayout layout,
            uint32_t set,
            uint32_t bufferIndex,   // set index is the index of the buffer that was returned by BindDescriptorBuffer(...)
            vk::DeviceSize offset,        // offset in the buffer
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eRayTracingKHR
        );

        void BindDescriptorSet(
            vk::PipelineLayout layout,
            uint32_t set,
            std::vector<uint32_t> bufferIndex,   // set index is the index of the buffer that was returned by BindDescriptorBuffer(...)
            std::vector<vk::DeviceSize> offset,        // offset in the descriptor buffer, that is bound at bufferIndex, to the descriptor set
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eRayTracingKHR
        );

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
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT mDescriptorBufferProperties;

        VmaAllocator mVMAllocator;

    };

    vk::AccelerationStructureGeometryDataKHR ConvertToVulkanGeometry(const GeometryData& geom);
    
} 

