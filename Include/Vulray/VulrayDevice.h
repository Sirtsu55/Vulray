#pragma once
#include "Vulray/Shader.h"
#include "Vulray/SBT.h"
#include "Vulray/AccelStruct.h"
#include "Vulray/Descriptors.h"

#ifdef VULRAY_BUILD_DENOISERS
#include "Vulray/Denoisers/DenoiserInterface.h"
#endif


namespace vr
{

    // Logging types
    enum class MessageType { Verbose, Info, Warning, Error};

    class VulrayDevice
    {
    public:
        
        /// @brief Creates a Vulray device to call all Vulray functions
        /// @param inst Handle to the Vulkan instance
        /// @param dev Handle to the Vulkan device
        /// @param physDev Handle to the Vulkan physical device
        VulrayDevice(vk::Instance inst, vk::Device dev, vk::PhysicalDevice physDev);
        ~VulrayDevice();

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@ Getter Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Get the Vulkan device handle
        vk::Device GetDevice() const { return mDevice; }

        /// @brief Get the Vulkan physical device handle
        vk::PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

        /// @brief Get the Vulkan instance handle
        vk::Instance GetInstance() const { return mInstance; }

        /// @brief Get the Physical device properties
        vk::PhysicalDeviceProperties GetProperties() const { return mDeviceProperties; }

        /// @brief Get the Ray Tracing properties of the physical device
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingProperties() const { return mRayTracingProperties; }

        /// @brief Get the Acceleration Structure properties of the physical device
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR GetAccelerationStructureProperties() const { return mAccelProperties; }

        /// @brief Get the Descriptor Buffer properties of the physical device
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT GetDescriptorBufferProperties() const { return mDescriptorBufferProperties; } 

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@ Command Buffer Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Create a Command Buffer
        /// @param pool The command pool that will be used to allocate the command buffer
        /// @param level The level of the command buffer
        /// @return The created command buffer
        [[nodiscard]] vk::CommandBuffer CreateCommandBuffer(vk::CommandPool pool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

        /// @brief Create multiple command buffers
        /// @param pool The command pool that will be used to allocate the command buffers
        /// @param count The number of command buffers to create
        /// @param level The level of the command buffers
        /// @return The created command buffers
        [[nodiscard]] std::vector<vk::CommandBuffer> CreateCommandBuffers(vk::CommandPool pool, uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

        /// @brief Transitions the image layout
        /// @param image The image that will be transitioned
        /// @param oldLayout The old layout of the image
        /// @param newLayout The new layout of the image
        /// @param range The subresource range of the image
        /// @param cmdBuf The command buffer that will be used to record the transition
        /// @param srcStage The source pipeline stage, default is all commands
        /// @param dstStage The destination pipeline stage, default is all commands
        void TransitionImageLayout(vk::Image image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout, 
            const vk::ImageSubresourceRange& range,
            vk::CommandBuffer cmdBuf,
            vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eAllGraphics,
            vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eAllCommands);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@ Acceleration Structure Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Creates a bottom level acceleration structure and gives BuildInfo for the build
        /// @param info The information that will be used to create the acceleration structure
        /// @return A pair of the acceleration structure handle and the build info
        [[nodiscard]] std::pair<BLASHandle, BLASBuildInfo> CreateBLAS(const BLASCreateInfo& info);

        /// @brief Builds the acceleration structure and records the build to the command buffer
        /// @param buildInfos Vector of build infos that will be used to build the acceleration structure, this should be the return value of CreateBLAS(...)
        /// @param cmdBuf The command buffer that will be used to record the build
        void BuildBLAS(const std::vector<BLASBuildInfo> &buildInfos, vk::CommandBuffer cmdBuf);

        /// @brief Updates the acceleration structure and returns the scratch buffer for building
        /// @param updateInfo The information that will be used to update the acceleration structure
        /// @return The build info that will be used to build the acceleration structure
        /// @note The BLAS needs to be built again with the returned build info
        [[nodiscard]] BLASBuildInfo UpdateBLAS(BLASUpdateInfo& updateInfo);

        /// @brief Creates a top level acceleration structure
        /// @param info The information that will be used to create the acceleration structure
        /// @return A pair of the acceleration structure handle and the build info
        [[nodiscard]] std::pair<TLASHandle, TLASBuildInfo> CreateTLAS(const TLASCreateInfo& info);

        /// @brief Builds the acceleration structure and records the build to the command buffer
        /// @param buildInfo The build info that will be used to build the acceleration structure, this should be the return value of CreateTLAS(...)
        /// @param InstanceBuffer The buffer that contains the instances that will be used to build the acceleration structure
        /// @param instanceCount The number of instances in the InstanceBuffer
        /// @param cmdBuf The command buffer that will be used to record the build
        void BuildTLAS(TLASBuildInfo& buildInfo, 
            const AllocatedBuffer& InstanceBuffer, uint32_t instanceCount, 
            vk::CommandBuffer cmdBuf);
            
        /// @brief Updates the acceleration structure
        /// @param oldTLAS The old acceleration structure that will be updated
        /// @param oldBuildInfo The old build info that will be used to update the acceleration structure
        /// @param destroyOld If true, the old acceleration structure will be destroyed after the update, default is true
        /// @return A pair of the acceleration structure handle and the build info
        /// @note This function does not strictly "Update" the acceleration structure, it creates a new acceleration structure that is identical to the old one.
        /// This is done because updating the acceleration structure degrades the quality of the acceleration structure over time.
        /// Top level acceleration structure's build time is negligible in a real time application, so it is recommended to create a new acceleration structure
        /// instead of updating the old one, according to NVIDIA's best practices.
        [[nodiscard]] std::pair<TLASHandle, TLASBuildInfo> UpdateTLAS(TLASHandle& oldTLAS, TLASBuildInfo& oldBuildInfo, bool destroyOld = true);

        /// @brief Creates a compaction request for the given BLASes
        /// @param sourceBLAS The pointer to the BLASes that will be compacted supplied in a vector
        /// @return The compaction request handle to call GetCompactionSizes(...) and CompactBLAS(...)
        [[nodiscard]] CompactionRequest RequestCompaction(const std::vector<BLASHandle*>& sourceBLAS);

        /// @brief Returns the sizes required for compaction
        /// @param request The compaction request that will be used to get the sizes
        /// @param cmdBuf The command buffer that will be used to record the query
        /// @return The sizes required for compaction
        /// @note 1. If the vector is not empty, then the query succeeded and the vector contains the compacted sizes
        /// 2. After this function succesfully returns the sizes, the user should call CompactBLAS(...)
        /// @warning 1. If the vector is empty, then the query failed and GetCompactionSizes(...) should be called again after command buffer execution
        /// 2. This function cannot be called again with the same CompactionRequest to get the sizes again after it succesfully returns the values,
        [[nodiscard]] std::vector<uint64_t> GetCompactionSizes(CompactionRequest& request, vk::CommandBuffer cmdBuf);

        /// @brief Compacts the BLASes and returns the compacted BLASes
        /// @param request The compaction request that will be used to compact the BLASes, this should be the return value of RequestCompaction(...)
        /// @param sizes The sizes that will be used to compact the BLASes, this should be the return value of GetCompactionSizes(...)
        /// @param cmdBuf The command buffer that will be used to record the compaction
        /// @return The compacted BLASes
        /// @note After this function succesfully returns the compacted BLASes, the user should destroy the source BLASes AFTER the command buffer execution
        [[nodiscard]] std::vector<BLASHandle> CompactBLAS(CompactionRequest& request, const std::vector<uint64_t>& sizes, vk::CommandBuffer cmdBuf);

        /// @brief Compacts the BLASes and returns the old BLASes to be destroyed
        /// @param request The compaction request that will be used to compact the BLASes, this should be the return value of RequestCompaction(...)
        /// @param sizes The sizes that will be used to compact the BLASes, this should be the return value of GetCompactionSizes(...)
        /// @param oldBLAS The old BLASes that will be replaced with the compacted BLASes
        /// @param cmdBuf The command buffer that will be used to record the compaction
        /// @return The old BLASes that should be destroyed after the command buffer execution
        /// @note After this function succesfully returns the old BLASes, the user should destroy the BLASes returned AFTER the command buffer execution
        [[nodiscard]] std::vector<BLASHandle> CompactBLAS(CompactionRequest& request, const std::vector<uint64_t>& sizes,
            std::vector<BLASHandle*> oldBLAS, vk::CommandBuffer cmdBuf);

        /// @brief Creates a SINGLE scratch buffer for building acceleration structures and binds the scratch buffer to the build infos
        /// @param buildInfos The build infos that will be used to create the scratch buffer
        /// @return The scratch buffer
        /// @note This function creates a single scratch buffer for ALL the BLASes in the build infos.
        /// If the BLAS is updated regularly, it is recommended to create a separate scratch buffer for the updating BLAS and use the scratch buffer for the updating.
        [[nodiscard]] AllocatedBuffer CreateScratchBufferBLAS(std::vector<BLASBuildInfo>& buildInfos);

        /// @brief Creates a SINGLE scratch buffer for building acceleration structures and binds the scratch buffer to the build infos
        /// @param buildInfo The build info that will be used to create the scratch buffer
        /// @return The scratch buffer
        [[nodiscard]] AllocatedBuffer CreateScratchBufferBLAS(BLASBuildInfo& buildInfo);

        /// @brief Creates a SINGLE scratch buffer for building acceleration structure, and binds the scratch buffer to the build info
        /// @param buildInfo The build info that will be used to create the scratch buffer
        /// @return The scratch buffer
        [[nodiscard]] AllocatedBuffer CreateScratchBufferTLAS(TLASBuildInfo& buildInfo);

        /// @brief Binds the scratch buffer to the build infos, buildInfo.BuildGeometryInfo.scratchData is set to be the scratchBuffer
        /// @param scratchBuffer The scratch buffer that will be bound
        /// @param buildInfos The build infos that will be bound to the scratch buffer
        [[nodiscard]] void BindScratchBufferToBuildInfo(vk::DeviceAddress scratchBuffer, BLASBuildInfo& buildInfo);

        /// @brief Binds the scratch buffer to the build infos, buildInfo.BuildGeometryInfo.scratchData is set to be the scratchBuffer
        /// @param scratchBuffer The scratch buffer that will be bound
        /// @param buildInfos The build infos that will be bound to the scratch buffer
        [[nodiscard]] void BindScratchBufferToBuildInfo(vk::DeviceAddress scratchBuffer, TLASBuildInfo& buildInfo);


        /// @brief Adds barrier to the command buffer to ensure the acceleration structure is built before other acceleration structures are built
        /// @param cmdBuf The command buffer that will be used to record the barrier
        void AddAccelerationBuildBarrier(vk::CommandBuffer cmdBuf);

        /// @brief Destroys the acceleration structure
        /// @param accel The acceleration structures that will be destroyed
        void DestroyBLAS(std::vector<BLASHandle>& blas);

        /// @brief Destroys the acceleration structure
        /// @param blas The acceleration structure that will be destroyed
        void DestroyBLAS(BLASHandle& blas);

        /// @brief Destroys the acceleration structure
        /// @param accel The acceleration structures that will be destroyed
        void DestroyTLAS(TLASHandle& tlas);

        /// @brief Destroys raw Vulkan acceleration structure
        void DestroyAccelerationStructure(const vk::AccelerationStructureKHR& accel);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@ Allocation Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Creates an Image
        /// @param imgInfo The information that will be used to create the image
        /// @param flags The VMA flags that will be used to allocate the image
        /// @return The created image
        /// @note Image views are not created in this function and must be created manually
        [[nodiscard]] AllocatedImage CreateImage(
            const vk::ImageCreateInfo& imgInfo,
            VmaAllocationCreateFlags flags);

        /// @brief Creates a buffer
        /// @param size The size of the buffer
        /// @param flags The VMA flags that will be used to allocate the buffer
        /// @param bufferUsage The usage of the buffer
        /// @param alignment The alignment of the buffer, default is no alignment
        /// @return The created buffer
        /// @note All the buffers are created with the eShaderDeviceAddressKHR flag, so no need to add it to the buffer usage.
        /// Furthermore, VmaAllocationCreateInfo::usage is VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, so the memory will be allocated preferentially on the device,
        /// do not confuse this with VmaAllocationCreateInfo::flags which is asked to be passed as a parameter.
        [[nodiscard]] AllocatedBuffer CreateBuffer(
            vk::DeviceSize size,
            VmaAllocationCreateFlags flags,
            vk::BufferUsageFlags bufferUsage,
            uint32_t alignment = 0);

        /// @brief Creates a buffer for storing the instances
        /// @param instanceCount The number of instances that will be stored in the buffer (not byte size)
        /// @return The created buffer
        /// @note The buffer is created with the VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT flag, so it is host writable.
        /// If you want it in device local memory, you should create a buffer with CreateBuffer(...) and copy the instance data to the device local buffer.
        [[nodiscard]] AllocatedBuffer CreateInstanceBuffer(uint32_t instanceCount);

        /// @brief Creates a buffer for storing the descriptor sets
        /// @param layout The descriptor set layout that will be used to create the buffer
        /// @param items The descriptor items that will be used to create the buffer
        /// @param type The type of the descriptor buffer
        /// @param setCount The number of descriptor sets that will be allocated for storage in the buffer, default is 1
        /// @return The created descriptor buffer
        [[nodiscard]] DescriptorBuffer CreateDescriptorBuffer(vk::DescriptorSetLayout layout,
            std::vector<DescriptorItem>& items,
            DescriptorBufferType type,
            uint32_t setCount = 1);

        /// @brief Copies data from src to dst via vkCmdCopyBuffer
        /// @param src The source buffer
        /// @param dst The destination buffer
        /// @param size The size in bytes of the data that will be copied
        /// @param cmdBuf The command buffer that will be used to record the copy
        /// @note This function is useful for staging buffers, where the data is copied from the staging buffer to the device local buffer.
        /// @warning This function does not check if the buffers are big enough to copy the data, it is the user's responsibility to check the sizes.
		void CopyData(AllocatedBuffer src, AllocatedBuffer dst, vk::DeviceSize size, vk::CommandBuffer cmdBuf);

        /// @brief Uploads data to a buffer, via mapping the buffer and memcpy
        /// @param alloc The buffer that will be updated, MUST be host visible when created
        /// @param data The data that will be copied to the buffer
        /// @param size The size in bytes of the data that will be copied to the buffer
        /// @param offset The offset in bytes of the buffer that will be updated, default is 0
        /// @note If doing many memcpy operations, it is recommended to map the buffer once (via MapBuffer(...)) and memcpy to the mapped data
        /// This function maps and unmaps the buffer every time it is called, therefore, it is inefficient to call this function many times
        /// @warning Segfault if pointer and size are not valid / out of bounds.
        /// VMA assertion if the buffer is not mappable.
        void UpdateBuffer(AllocatedBuffer alloc, void* data, const vk::DeviceSize size, uint32_t offset = 0);

        /// @brief Maps the buffer and returns the mapped data
        /// @param buffer The buffer that will be mapped
        /// @return The mapped data
        /// @note The buffer must have been created with the VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT flag or similar flags
        [[nodiscard]] void* MapBuffer(AllocatedBuffer &buffer);

        /// @brief Unmaps the buffer
        /// @param buffer The buffer that will be unmapped
        void UnmapBuffer(AllocatedBuffer &buffer);
        
        /// @brief Destroys the buffer
        /// @param buffer The buffer that will be destroyed
        void DestroyBuffer(AllocatedBuffer& buffer);

        /// @brief Destroys the image
        /// @param img The image that will be destroyed
        void DestroyImage(AllocatedImage & img);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@ Pipeline Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Creates a shader object from SPIRV code
        /// @param info The information that will be used to create the shader module
        /// @return The created shader module
        [[nodiscard]] Shader CreateShaderFromSPV(ShaderCreateInfo& info);

        /// @brief Creates a shader module from SPIRV code
        /// @param spvCode The SPIRV code that will be used to create the shader module
        /// @return The created shader module
        [[nodiscard]] vk::ShaderModule CreateShaderModule(const std::vector<uint32_t>& spvCode);


        /// @brief Creates a pipeline layout
        /// @param descLayout The descriptor set layout that will be used to create the pipeline layout
        /// @return The created pipeline layout
        [[nodiscard]] vk::PipelineLayout CreatePipelineLayout(vk::DescriptorSetLayout descLayout);

        /// @brief Creates a pipeline layout
        /// @param descLayouts The descriptor set layouts that will be used to create the pipeline layout
        /// @return The created pipeline layout
        [[nodiscard]] vk::PipelineLayout CreatePipelineLayout(const std::vector<vk::DescriptorSetLayout>& descLayouts);

        /// @brief Creates a pipeline
        /// @param descLayouts The descriptor set layouts that will be used to create the pipeline layout
        /// @param info The ShaderBindingTable that will be used to create the pipeline layout
        /// @param recursionDepth The recursion depth of the ray tracing pipeline
        /// @param flags The flags that will be used to create the pipeline. If using Vulray's descriptor buffer, this should have the eDescriptorBufferEXT flag.
        /// @return The created pipeline
        [[nodiscard]] vk::Pipeline CreateRayTracingPipeline(vk::PipelineLayout layout, const ShaderBindingTable& info, uint32_t recursuionDepth, vk::PipelineCreateFlags flags = vk::PipelineCreateFlagBits::eDescriptorBufferEXT );

        /// @brief Destroys the shader module
        /// @param shader The shader module that will be destroyed
        void DestroyShader(Shader & shader);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@ Descriptor Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Creates a descriptor set layout
        /// @param bindings The descriptor items that will be used to create the descriptor set layout
        /// @return The created descriptor set layout
        [[nodiscard]] vk::DescriptorSetLayout CreateDescriptorSetLayout(const std::vector<DescriptorItem> &bindings);

        /// @brief Updates the descriptor buffer with the descriptor items in the set
        /// @param buffer The descriptor buffer that will be updated
        /// @param items The descriptor items that will be used to update the descriptor buffer
        /// @param type The type of the descriptor buffer
        /// @param setIndexInBuffer The index of the descriptor set in the buffer, default is 0
        /// @param pMappedData The pointer to the mapped data of the buffer, if it is null, the buffer will be mapped and unmapped, default is nullptr
        /// @note For each element in the items vector, the buffer will be updated with the given data in DescriptorItem::pImageViews/pResources pointer
        /// @warning There can be a segmentation fault if the pointers in the DescriptorItem are not valid or the pointers 
        /// are not pointing to an array of DescriptorItem::ArraySize/DynamicArraySize elements
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer,
            const std::vector<DescriptorItem>& items,
            DescriptorBufferType type,
            uint32_t setIndexInBuffer = 0,
            void* pMappedData = nullptr);


        /// @brief Updates the descriptor buffer with the single descriptor item including all the elements in the array
        /// @param buffer The descriptor buffer that will be updated
        /// @param item The descriptor item that will be used to update the descriptor buffer
        /// @param type The type of the descriptor buffer
        /// @param setIndexInBuffer The index of the descriptor set in the buffer, default is 0
        /// @param pMappedData The pointer to the mapped data of the buffer, if it is null, the buffer will be mapped and unmapped, default is nullptr
        /// @note For each element in the items vector, the buffer will be updated with the given data in DescriptorItem::pImageViews/pResources pointer
        /// @warning There can be a segmentation fault if the pointers in the DescriptorItem are not valid or the pointers
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer,
            const DescriptorItem& item,
            DescriptorBufferType type,
            uint32_t setIndexInBuffer = 0,
            void* pMappedData = nullptr);

        /// @brief Updates the descriptor buffer with one element of the descriptor item
        /// @param buffer The descriptor buffer that will be updated
        /// @param item The descriptor item that will be used to update the descriptor buffer
        /// @param itemIndex The index of the descriptor item in the layout -> DescriptorItem::p***[itemIndex]
        /// @param type The type of the descriptor buffer
        /// @param setIndexInBuffer The index of the descriptor set in the buffer, default is 0
        /// @param pMappedData The pointer to the mapped data of the buffer, if it is null, the buffer will be mapped and unmapped, default is nullptr
        /// @warning There can be a segmentation fault if the pointers in the DescriptorItem are not valid or the item index is out of bounds
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer,
            const DescriptorItem& item,
            uint32_t itemIndex,
            DescriptorBufferType type,
            uint32_t setIndexInBuffer = 0,
            void* pMappedData = nullptr);

        /// @brief Binds the descriptor buffer to the command buffer
        /// @param buffers The descriptor buffers that will be bound
        /// @param cmdBuf The command buffer that will be used to record the bind
        void BindDescriptorBuffer(const std::vector<DescriptorBuffer>& buffers, vk::CommandBuffer cmdBuf);

        /// @brief Binds the descriptor set to the command buffer
        /// @param layout The pipeline layout that will be used to bind the descriptor set
        /// @param set The set where the descriptor set will be bound
        /// @param bufferIndex The index of the descriptor set in the descriptor buffer that will be bound.
        /// This is the index of the descriptor buffer that was given to BindDescriptorBuffer(...) function that will be bound
        /// @param offset The offset to where the buffer starts in the descriptor buffer that will be bound to the descriptor set
        /// If multiple descriptor sets are bound to the same descriptor buffer, this is the offset to the start of the descriptor set
        /// that will be bound
        /// @param cmdBuf The command buffer that will be used to record the bind
        /// @param bindPoint The bind point of the descriptor set, default is eRayTracingKHR
        void BindDescriptorSet(
            vk::PipelineLayout layout,
            uint32_t set,
            uint32_t bufferIndex,
            vk::DeviceSize offset,
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eRayTracingKHR
        );

        /// @brief Binds the descriptor sets to the command buffer
        /// @param layout The pipeline layout that will be used to bind the descriptor set
        /// @param set The first set where the descriptor set will be bound
        /// @param bufferIndex The indices of the descriptor sets in the descriptor buffer that will be bound.
        /// This is the index of the descriptor buffer that was given to BindDescriptorBuffer(...) function that will be bound
        /// @param offset The offsets to where the buffer starts in the descriptor buffer that will be bound to the descriptor set
        /// If multiple descriptor sets are bound to the same descriptor buffer, this is the offset to the start of the descriptor set
        /// that will be bound
        /// @param cmdBuf The command buffer that will be used to record the bind
        /// @param bindPoint The bind point of the descriptor set, default is eRayTracingKHR
        void BindDescriptorSet(
            vk::PipelineLayout layout,
            uint32_t set,
            std::vector<uint32_t> bufferIndex,
            std::vector<vk::DeviceSize> offset,  // offset in the descriptor buffer, that is bound at bufferIndex, to the descriptor set
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eRayTracingKHR
        );

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@ Shader Binding Table Functions @@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Gets the opaque handles for the shader records in the SBT buffer
        /// @param pipeline The pipeline that will be used to get the handles
        /// @param firstGroup The index of the first shader group in the pipeline
        /// @param groupCount The number of shader groups that will be used to get the handles
        /// @return The opaque handles for the shader records in the SBT buffer
        /// @note For Vulray's internal use, but can be used by the user doing custom SBT 
        [[nodiscard]] std::vector<uint8_t> GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup, uint32_t groupCount);

        /// @brief Writes data to a shader record in the SBT buffer
        /// @param sbtBuf The SBT buffer that will be written to
        /// @param group The shader group that will be written to
        /// @param groupIndex The index of the shader group that will be written to
        /// @param data The data that will be written to the shader record
        /// @param dataSize The size of the data in bytes that will be written to the shader record
        /// @param mappedData The pointer to the mapped data of the SBT buffer, if it is null, the buffer will be mapped and unmapped, default is nullptr
        /// @warning Segfault if any of the pointers are not valid or the data size if out of bounds
        void WriteToSBT(SBTBuffer sbtBuf, ShaderGroup group, uint32_t groupIndex, void* data, uint32_t dataSize, void* mappedData = nullptr);


        /// @brief Creates a buffer for each shader group in the pipeline and copies the opaque handles to the buffer
        /// @param pipeline The pipeline that will be used to create the SBT buffer
        /// @param sbt The collection of shaders that will be used to create the SBT buffer
        /// @return The created SBT buffer
        [[nodiscard]] SBTBuffer CreateSBT(vk::Pipeline pipeline, const ShaderBindingTable& sbt);

        /// @brief Destroys the SBT buffer
        /// @param buffer The SBT buffer that will be destroyed
        void DestroySBTBuffer(SBTBuffer& buffer);

        /// @brief Dispatches the rays
        /// @param cmdBuf The command buffer that will be used to record the dispatch
        /// @param rtPipeline The ray tracing pipeline that will be used to dispatch the rays
        /// @param buffer The SBT buffer that contains the shader records
        /// @param width The width of the image that will be used to dispatch the rays
        /// @param height The height of the image that will be used to dispatch the rays
        /// @param depth The depth of the image that will be used to dispatch the rays, default is 1
        void DispatchRays(vk::CommandBuffer cmdBuf, const vk::Pipeline rtPipeline, const SBTBuffer& buffer, uint32_t width, uint32_t height, uint32_t depth = 1);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@ Denoiser Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifdef VULRAY_BUILD_DENOISERS

        /// @brief Creates a denoiser of type T
        /// @tparam T The type of the denoiser that will be created - Found in Denoisers namespace. will fail to compile if T is not a denoiser
        template <typename T, typename std::enable_if<std::is_base_of<Denoise::DenoiserInterface, T>::value, int>::type = 0>
        vr::Denoiser CreateDenoiser(int width, int height)
        {
            return std::make_unique<T>(this, width, height);
        }

#endif
        

    private:


        vk::DispatchLoaderDynamic mDynLoader;

        vk::Instance mInstance;
        vk::Device mDevice;
        vk::PhysicalDevice mPhysicalDevice;
        
        vk::PhysicalDeviceProperties mDeviceProperties;
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR mRayTracingProperties;
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR mAccelProperties;
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT mDescriptorBufferProperties;

        VmaAllocator mVMAllocator;

    };

    vk::AccelerationStructureGeometryDataKHR ConvertToVulkanGeometry(const GeometryData& geom);
    
} 

