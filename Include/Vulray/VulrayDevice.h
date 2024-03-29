#pragma once

#include "Vulray/AccelStruct.h"
#include "Vulray/Descriptors.h"
#include "Vulray/SBT.h"
#include "Vulray/Shader.h"

#ifdef VULRAY_BUILD_DENOISERS
#include "Vulray/Denoisers/DenoiserInterface.h"
#endif

namespace vr
{

    // Logging types
    enum class MessageType
    {
        Verbose,
        Info,
        Warning,
        Error
    };

    class VulrayDevice
    {
      public:
        /// @brief Creates a Vulray device to call all Vulray functions
        /// @param inst Handle to the Vulkan instance
        /// @param dev Handle to the Vulkan device
        /// @param physDev Handle to the Vulkan physical device
        /// @param allocator The VMA allocator that will be used to allocate memory, if it is nullptr then a new
        /// allocator for VMA will be created, and destroyed when the device is destroyed. If an allocator is passed in
        /// then it's not destroyed when the device is destroyed.
        VulrayDevice(vk::Instance inst, vk::Device dev, vk::PhysicalDevice physDev, VmaAllocator allocator = nullptr);
        ~VulrayDevice();

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@ Setter Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Set the VmaPool that will be used to allocate memory internally
        /// @param pool The VmaPool that will be used to allocate memory internally, can be nullptr to use the default
        /// @note When creating descriptor buffers / images / acceleleration structures, the pool will be used to
        /// allocate the memory so that means the the pool MUST have been created to support these types of allocations.
        /// The allocations end up anywhere physically: Device or Host depending on the pool.
        /// @warning If the pool is not created with the correct flags / memory types, then the allocations will fail.
        void SetVmaPool(VmaPool pool) { mCurrentPool = pool; }

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@ Getter Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Get the Vulkan device handle
        vk::Device GetDevice() const { return mDevice; }

        /// @brief Get the Vulkan physical device handle
        vk::PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

        /// @brief Get the Vulkan instance handle
        vk::Instance GetInstance() const { return mInstance; }

        /// @brief Get the Vulkan dynamic loader
        vk::DispatchLoaderDynamic GetDynamicLoader() const { return mDynLoader; }

        /// @brief Get the Physical device properties
        vk::PhysicalDeviceProperties GetProperties() const { return mDeviceProperties; }

        /// @brief Get the Ray Tracing properties of the physical device
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingProperties() const
        {
            return mRayTracingProperties;
        }

        /// @brief Get the Acceleration Structure properties of the physical device
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR GetAccelerationStructureProperties() const
        {
            return mAccelProperties;
        }

        /// @brief Get the Descriptor Buffer properties of the physical device
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT GetDescriptorBufferProperties() const
        {
            return mDescriptorBufferProperties;
        }

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@ Command Buffer Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Transitions the image layout
        /// @param image The image that will be transitioned
        /// @param oldLayout The old layout of the image
        /// @param newLayout The new layout of the image
        /// @param range The subresource range of the image
        /// @param cmdBuf The command buffer that will be used to record the transition
        /// @param srcStage The source pipeline stage, default is all commands
        /// @param dstStage The destination pipeline stage, default is all commands
        void TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                   const vk::ImageSubresourceRange& range, vk::CommandBuffer cmdBuf,
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
        /// @param buildInfos Vector of build infos that will be used to build the acceleration structure, this should
        /// be the return value of CreateBLAS(...)
        /// @param cmdBuf The command buffer that will be used to record the build
        void BuildBLAS(const std::vector<BLASBuildInfo>& buildInfos, vk::CommandBuffer cmdBuf);

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
        /// @param buildInfo The build info that will be used to build the acceleration structure, this should be the
        /// return value of CreateTLAS(...)
        /// @param InstanceBuffer The buffer that contains the instances that will be used to build the acceleration
        /// structure
        /// @param instanceCount The number of instances in the InstanceBuffer
        /// @param cmdBuf The command buffer that will be used to record the build
        void BuildTLAS(TLASBuildInfo& buildInfo, const AllocatedBuffer& InstanceBuffer, uint32_t instanceCount,
                       vk::CommandBuffer cmdBuf);

        /// @brief Updates the acceleration structure
        /// @param oldTLAS The old acceleration structure that will be updated
        /// @param oldBuildInfo The old build info that will be used to update the acceleration structure
        /// @param destroyOld If true, the old acceleration structure will be destroyed after the update, default is
        /// true
        /// @return A pair of the acceleration structure handle and the build info
        /// @note This function does not strictly "Update" the acceleration structure, it creates a new acceleration
        /// structure that is identical to the old one. This is done because updating the acceleration structure
        /// degrades the quality of the acceleration structure over time. Top level acceleration structure's build time
        /// is negligible in a real time application, so it is recommended to create a new acceleration structure
        /// instead of updating the old one, according to NVIDIA's best practices.
        [[nodiscard]] std::pair<TLASHandle, TLASBuildInfo> UpdateTLAS(TLASHandle& oldTLAS, TLASBuildInfo& oldBuildInfo,
                                                                      bool destroyOld = true);

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
        /// @warning 1. If the vector is empty, then the query failed and GetCompactionSizes(...) should be called again
        /// after command buffer execution
        /// 2. This function cannot be called again with the same CompactionRequest to get the sizes again after it
        /// succesfully returns the values,
        [[nodiscard]] std::vector<uint64_t> GetCompactionSizes(CompactionRequest& request, vk::CommandBuffer cmdBuf);

        /// @brief Compacts the BLASes and returns the compacted BLASes
        /// @param request The compaction request that will be used to compact the BLASes, this should be the return
        /// value of RequestCompaction(...)
        /// @param sizes The sizes that will be used to compact the BLASes, this should be the return value of
        /// GetCompactionSizes(...)
        /// @param cmdBuf The command buffer that will be used to record the compaction
        /// @return The compacted BLASes
        /// @note After this function succesfully returns the compacted BLASes, the user should destroy the source
        /// BLASes AFTER the command buffer execution
        [[nodiscard]] std::vector<BLASHandle> CompactBLAS(CompactionRequest& request,
                                                          const std::vector<uint64_t>& sizes, vk::CommandBuffer cmdBuf);

        /// @brief Compacts the BLASes and returns the old BLASes to be destroyed
        /// @param request The compaction request that will be used to compact the BLASes, this should be the return
        /// value of RequestCompaction(...)
        /// @param sizes The sizes that will be used to compact the BLASes, this should be the return value of
        /// GetCompactionSizes(...)
        /// @param oldBLAS The old BLASes that will be replaced with the compacted BLASes
        /// @param cmdBuf The command buffer that will be used to record the compaction
        /// @return The old BLASes that should be destroyed after the command buffer execution
        /// @note After this function succesfully returns the old BLASes, the user should destroy the BLASes returned
        /// AFTER the command buffer execution
        [[nodiscard]] std::vector<BLASHandle> CompactBLAS(CompactionRequest& request,
                                                          const std::vector<uint64_t>& sizes,
                                                          std::vector<BLASHandle*> oldBLAS, vk::CommandBuffer cmdBuf);

        /// @brief Creates a SINGLE scratch buffer for building acceleration structures and binds the scratch buffer to
        /// the build infos
        /// @param buildInfos The build infos that will be used to create the scratch buffer
        /// @return The scratch buffer
        /// @note This function creates a single scratch buffer for ALL the BLASes in the build infos.
        /// If the BLAS is updated regularly, it is recommended to create a separate scratch buffer for the updating
        /// BLAS and use the scratch buffer for the updating.
        [[nodiscard]] AllocatedBuffer CreateScratchBufferFromBuildInfos(std::vector<BLASBuildInfo>& buildInfos);

        /// @brief Binds the scratch buffer to the build info
        /// @param scratchBuffer The scratch buffer that will be bound
        /// @param buildInfo The build info that will be bound to the scratch buffer
        [[nodiscard]] AllocatedBuffer CreateScratchBufferFromBuildInfo(BLASBuildInfo& buildInfo);

        /// @brief Creates a SINGLE scratch buffer for building acceleration structure, and binds the scratch buffer to
        /// the build info
        /// @param buildInfo The build info that will be used to create the scratch buffer
        /// @return The scratch buffer
        [[nodiscard]] AllocatedBuffer CreateScratchBufferFromBuildInfos(std::vector<TLASBuildInfo>& buildInfo);

        /// @brief Binds the scratch buffer to the build info
        /// @param scratchBuffer The scratch buffer that will be bound
        /// @param buildInfo The build info that will be bound to the scratch buffer
        [[nodiscard]] AllocatedBuffer CreateScratchBufferFromBuildInfo(TLASBuildInfo& buildInfo);

        /// @brief Binds the scratch buffer to the build infos. Assumes that the whole scratch buffer is used for all
        /// the build infos. This function is useful, when you already have a scratch buffer and you want to use it for
        /// a build. Ideally one should create a big scratch buffer and use it for all the builds, so this allows that.
        /// @param scratchBuffer The scratch buffer that will be bound
        /// @param buildInfos The build infos that will be bound to the scratch buffer
        /// @warning This function doesn't check if the scratch buffer is big enough to build the acceleration
        /// structure, it is the user's responsibility to check the sizes. and ensure it doesn't overflow, Use
        /// GetScratchBufferSize(...) to get required size of the scratch buffer.
        void BindScratchBufferToBuildInfos(const vr::AllocatedBuffer& buffer, std::vector<BLASBuildInfo>& buildInfos);

        /// @brief Binds the scratch buffer to the build infos. Assumes that the whole scratch buffer is used for all
        /// the build infos. This function is useful, when you already have a scratch buffer and you want to use it for
        /// a build. Ideally one should create a big scratch buffer and use it for all the builds, so this allows that.
        /// @param scratchBuffer The scratch buffer that will be bound
        /// @param buildInfos The build infos that will be bound to the scratch buffer
        /// @warning This function doesn't check if the scratch buffer is big enough to build the acceleration
        /// structure, it is the user's responsibility to check the sizes. and ensure it doesn't overflow, Use
        /// GetScratchBufferSize(...) to get required size of the scratch buffer.
        void BindScratchBufferToBuildInfos(const vr::AllocatedBuffer& buffer, std::vector<TLASBuildInfo>& buildInfos);

        /// @brief Binds the scratch buffer to the build info.
        /// @param scratchAddr The scratch address that will be bound
        /// @param buildInfo The build info that will be bound to the scratch address
        void BindScratchAdressToBuildInfo(vk::DeviceAddress scratchAddr, BLASBuildInfo& buildInfo);

        /// @brief Binds the scratch buffer to the build info.
        /// @param scratchAddr The scratch address that will be bound
        /// @param buildInfo The build info that will be bound to the scratch address
        void BindScratchAdressToBuildInfo(vk::DeviceAddress scratchAddr, TLASBuildInfo& buildInfo);

        /// @brief Returns the size of the scratch buffer required to build all the acceleration structures.
        /// @param infos The build infos that will be used to get the size of the scratch buffer
        /// @return The size of the scratch buffer required to build all the acceleration structures
        [[nodiscard]] uint32_t GetScratchBufferSize(const std::vector<BLASBuildInfo>& buildInfos);

        /// @brief Returns the size of the scratch buffer required to build all the acceleration structures.
        /// @param infos The build infos that will be used to get the size of the scratch buffer
        /// @return The size of the scratch buffer required to build all the acceleration structures
        [[nodiscard]] uint32_t GetScratchBufferSize(const std::vector<TLASBuildInfo>& buildInfos);

        /// @brief Adds barrier to the command buffer to ensure the acceleration structure is built before other
        /// acceleration structures are built
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

        /// @brief Returns the VMA allocator that is used to allocate the resources
        VmaAllocator GetAllocator() const { return mVMAllocator; }

        /// @brief Creates an Image
        /// @param imgInfo The information that will be used to create the image
        /// @param flags The VMA flags that will be used to allocate the image
        /// @return The created image
        /// @note Image views are not created in this function and must be created manually
        [[nodiscard]] AllocatedImage CreateImage(const vk::ImageCreateInfo& imgInfo, VmaAllocationCreateFlags flags,
                                                 VmaPool pool = nullptr);

        /// @brief Creates a buffer
        /// @param size The size of the buffer
        /// @param bufferUsage The usage of the buffer
        /// @param flags The VMA flags that will be used to allocate the buffer
        /// @param alignment The alignment of the buffer, default is no alignment
        /// @param pool The VMA pool that will be used to allocate the buffer, if nullptr, the default pool will be
        /// used.
        /// @return The created buffer
        /// @note 1. All the buffers are created with the eShaderDeviceAddressKHR flag.
        /// 2. By default VmaAllocationCreateInfo::usage is VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, so the memory will be
        /// allocated preferentially on the device. This can be overriden by specifying a VmaPool from where the memory
        /// will be allocated.
        [[nodiscard]] AllocatedBuffer CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags bufferUsage,
                                                   VmaAllocationCreateFlags flags = 0, uint32_t alignment = 0,
                                                   VmaPool pool = nullptr);

        /// @brief Creates a buffer for storing the instances
        /// @param instanceCount The number of instances that will be stored in the buffer (not byte size)
        /// @return The created buffer
        /// @note The buffer is created with the VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT flag, so it is
        /// host writable. If you want it in device local memory, you should create a buffer with CreateBuffer(...) and
        /// copy the instance data to the device local buffer.
        [[nodiscard]] AllocatedBuffer CreateInstanceBuffer(uint32_t instanceCount);

        /// @brief Creates a buffer for storing the scratch data and uses correct alignment / flags
        /// @param size The size of the buffer
        /// @return The created buffer
        [[nodiscard]] AllocatedBuffer CreateScratchBuffer(uint32_t size);

        /// @brief Creates a buffer for storing the descriptor sets
        /// @param layout The descriptor set layout that will be used to create the buffer
        /// @param items The descriptor items that will be used to create the buffer
        /// @param type The type of the descriptor buffer
        /// @param setCount The number of descriptor sets that will be allocated for storage in the buffer, default is 1
        /// @return The created descriptor buffer
        [[nodiscard]] DescriptorBuffer CreateDescriptorBuffer(vk::DescriptorSetLayout layout,
                                                              std::vector<DescriptorItem>& items,
                                                              DescriptorBufferType type, uint32_t setCount = 1);

        /// @brief Copies data from src to dst via vkCmdCopyBuffer
        /// @param src The source buffer
        /// @param dst The destination buffer
        /// @param size The size in bytes of the data that will be copied
        /// @param cmdBuf The command buffer that will be used to record the copy
        /// @note This function is useful for staging buffers, where the data is copied from the staging buffer to the
        /// device local buffer.
        /// @warning This function does not check if the buffers are big enough to copy the data, it is the user's
        /// responsibility to check the sizes.
        void CopyData(AllocatedBuffer src, AllocatedBuffer dst, vk::DeviceSize size, vk::CommandBuffer cmdBuf);

        /// @brief Uploads data to a buffer, via mapping the buffer and memcpy
        /// @param alloc The buffer that will be updated, MUST be host visible when created
        /// @param data The data that will be copied to the buffer
        /// @param size The size in bytes of the data that will be copied to the buffer
        /// @param offset The offset in bytes of the buffer that will be updated, default is 0
        /// @note If doing many memcpy operations, it is recommended to map the buffer once (via MapBuffer(...)) and
        /// memcpy to the mapped data This function maps and unmaps the buffer every time it is called, therefore, it is
        /// inefficient to call this function many times
        /// @warning Segfault if pointer and size are not valid / out of bounds.
        /// VMA assertion if the buffer is not mappable.
        void UpdateBuffer(AllocatedBuffer alloc, void* data, const vk::DeviceSize size, uint32_t offset = 0);

        /// @brief Maps the buffer and returns the mapped data
        /// @param buffer The buffer that will be mapped
        /// @return The mapped data
        /// @note The buffer must have been created with the VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT flag
        /// or similar flags
        [[nodiscard]] void* MapBuffer(AllocatedBuffer& buffer);

        /// @brief Unmaps the buffer
        /// @param buffer The buffer that will be unmapped
        void UnmapBuffer(AllocatedBuffer& buffer);

        /// @brief Destroys the buffer
        /// @param buffer The buffer that will be destroyed
        void DestroyBuffer(AllocatedBuffer& buffer);

        /// @brief Destroys the image
        /// @param img The image that will be destroyed
        void DestroyImage(AllocatedImage& img);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@ Pipeline Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Creates a shader object from SPIRV code
        /// @param info The information that will be used to create the shader module
        /// @return The created shader module
        [[nodiscard]] Shader CreateShaderFromSPV(const std::vector<uint32_t>& spv);

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

        /// @brief Returns the shader stages and shader groups that are constructed from the ShaderBindingTable.
        /// Useful if wanting to create a pipeline library and link the pipeline library to the pipeline.
        /// @param info The ShaderBindingTable including the collection of shaders that will be used to create the
        /// shader stages and shader groups
        /// @return The shader stages and shader groups that are constructed from the ShaderBindingTable
        [[nodiscard]] std::pair<std::vector<vk::PipelineShaderStageCreateInfo>,
                                std::vector<vk::RayTracingShaderGroupCreateInfoKHR>>
        GetShaderStagesAndRayTracingGroups(const RayTracingShaderCollection& info);

        /// @brief Creates a ray tracing pipeline
        /// @param shaderCollection The shader collection that will be used to create the pipeline.
        /// The field shaderCollection::CollectionPipeline is not used, because it is not linking
        /// many pipelines together, it is just creating one pipeline.
        /// @param settings The settings that will be used to create the pipeline
        /// @param flags The flags that will be used to create the pipeline, default is eDescriptorBufferEXT
        /// @param deferredOp The deferred operation that will be used to create the pipeline, default is nullptr
        /// @return The created ray tracing pipeline and the shader binding table info to create the shader binding
        /// table
        [[nodiscard]] std::pair<vk::Pipeline, SBTInfo> CreateRayTracingPipeline(
            const RayTracingShaderCollection& shaderCollection, PipelineSettings& settings,
            vk::PipelineCreateFlags flags = vk::PipelineCreateFlagBits::eDescriptorBufferEXT,
            vk::DeferredOperationKHR deferredOp = nullptr);

        /// @brief Creates a ray tracing pipeline
        /// @param shaderCollections The shader collections that will be used to create the pipeline.
        /// The field shaderCollection::CollectionPipeline must be set, because it is linking many pipelines together.
        /// @param settings The settings that will be used to create the pipeline. All the pipelines in the
        /// shaderCollections must have been created with the same settings.
        /// @param flags The flags that will be used to create the pipeline, default is eDescriptorBufferEXT
        /// @param cache The pipeline cache that will be used to create the pipeline, default is nullptr
        /// @param deferredOp The deferred operation that will be used to create the pipeline, default is nullptr
        /// @return The created ray tracing pipeline and the shader binding table info to create the shader binding
        /// table
        [[nodiscard]] std::pair<vk::Pipeline, SBTInfo> CreateRayTracingPipeline(
            const std::vector<RayTracingShaderCollection>& shaderCollections, PipelineSettings& settings,
            vk::PipelineCreateFlags flags = vk::PipelineCreateFlagBits::eDescriptorBufferEXT,
            vk::PipelineCache cache = nullptr, vk::DeferredOperationKHR deferredOp = nullptr);

        /// @brief Convenience function that calls CreateRayTracingPipeline(...) and then copies the shader record sizes
        /// to the shader binding table info from the old shader binding table info to the new shader binding table
        /// info, so you don't have to set the shader record sizes again when creating the shader binding table.
        /// @param shaderCollection The shader collection that will be used to create the pipeline.
        /// many pipelines together, it is just creating one pipeline.
        /// @param settings The settings that will be used to create the pipeline
        /// @param sbtInfoOld The old shader binding table info that will be used to create the new shader binding table
        /// info
        /// @param flags The flags that will be used to create the pipeline, default is eDescriptorBufferEXT
        /// @param cache The pipeline cache that will be used to create the pipeline, default is nullptr
        /// @param deferredOp The deferred operation that will be used to create the pipeline, default is nullptr
        /// @return The created ray tracing pipeline and the shader binding table info to create the shader binding
        /// table
        [[nodiscard]] std::pair<vk::Pipeline, SBTInfo> CreateRayTracingPipeline(
            const std::vector<RayTracingShaderCollection>& shaderCollections, PipelineSettings& settings,
            SBTInfo& sbtInfoOld, vk::PipelineCreateFlags flags = vk::PipelineCreateFlagBits::eDescriptorBufferEXT,
            vk::PipelineCache cache = nullptr, vk::DeferredOperationKHR deferredOp = nullptr);

        /// @todo If an issue to anyone, then add support for creation of pipelines even when shaders are destroyed.
        /// in theory this is already possible, because we don't use the modules for anything other than pipeline
        /// library creation and we just need to know how many shaders per shader type were in the pipeline library. But
        /// the if someone doesn't like the fact that std::vector<Shader> is used because it consumes unnecessary
        /// memory, then this we can add support for this by adding a new struct that contains the number of shaders
        /// that were in the pipeline library and the pipeline library handle. Should be simple to implement, but I
        /// don't see a reason to do it unless someone requests it.

        /// @brief Creates a ray tracing pipeline library
        /// @param shaderCollection The shader collection that will be used to create the pipeline library
        /// @param settings The settings that will be used to create the pipeline library
        /// @param flags The flags that will be used to create the pipeline library, default is eDescriptorBufferEXT
        /// @param cache The pipeline cache that will be used to create the pipeline library, default is nullptr
        /// @param deferredOp The deferred operation that will be used to create the pipeline library, default is
        /// nullptr
        /// @return shaderCollection::CollectionPipeline is set to the created pipeline library
        void CreatePipelineLibrary(RayTracingShaderCollection& shaderCollection, PipelineSettings& settings,
                                   vk::PipelineCreateFlags flags = vk::PipelineCreateFlagBits::eDescriptorBufferEXT,
                                   vk::PipelineCache cache = nullptr, vk::DeferredOperationKHR deferredOp = nullptr);

        /// @brief Destroys the shader module
        /// @param shader The shader module that will be destroyed
        void DestroyShader(Shader& shader);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@ Descriptor Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Creates a descriptor set layout
        /// @param bindings The descriptor items that will be used to create the descriptor set layout
        /// @return The created descriptor set layout
        [[nodiscard]] vk::DescriptorSetLayout CreateDescriptorSetLayout(const std::vector<DescriptorItem>& bindings);

        /// @brief Updates the descriptor buffer with the descriptor items in the set
        /// @param buffer The descriptor buffer that will be updated
        /// @param items The descriptor items that will be used to update the descriptor buffer
        /// @param type The type of the descriptor buffer
        /// @param setIndexInBuffer The index of the descriptor set in the buffer, default is 0
        /// @param pMappedData The pointer to the mapped data of the buffer, if it is null, the buffer will be mapped
        /// and unmapped, default is nullptr
        /// @note For each element in the items vector, the buffer will be updated with the given data in
        /// DescriptorItem::pImageViews/pResources pointer
        /// @warning There can be a segmentation fault if the pointers in the DescriptorItem are not valid or the
        /// pointers are not pointing to an array of DescriptorItem::ArraySize/DynamicArraySize elements
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer, const std::vector<DescriptorItem>& items,
                                    DescriptorBufferType type, uint32_t setIndexInBuffer = 0,
                                    void* pMappedData = nullptr);

        /// @brief Updates the descriptor buffer with the single descriptor item including all the elements in the array
        /// @param buffer The descriptor buffer that will be updated
        /// @param item The descriptor item that will be used to update the descriptor buffer
        /// @param type The type of the descriptor buffer
        /// @param setIndexInBuffer The index of the descriptor set in the buffer, default is 0
        /// @param pMappedData The pointer to the mapped data of the buffer, if it is null, the buffer will be mapped
        /// and unmapped, default is nullptr
        /// @note For each element in the items vector, the buffer will be updated with the given data in
        /// DescriptorItem::pImageViews/pResources pointer
        /// @warning There can be a segmentation fault if the pointers in the DescriptorItem are not valid or the
        /// pointers
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer, const DescriptorItem& item, DescriptorBufferType type,
                                    uint32_t setIndexInBuffer = 0, void* pMappedData = nullptr);

        /// @brief Updates the descriptor buffer with one element of the descriptor item
        /// @param buffer The descriptor buffer that will be updated
        /// @param item The descriptor item that will be used to update the descriptor buffer
        /// @param itemIndex The index of the descriptor item in the layout -> DescriptorItem::p***[itemIndex]
        /// @param type The type of the descriptor buffer
        /// @param setIndexInBuffer The index of the descriptor set in the buffer, default is 0
        /// @param pMappedData The pointer to the mapped data of the buffer, if it is null, the buffer will be mapped
        /// and unmapped, default is nullptr
        /// @warning There can be a segmentation fault if the pointers in the DescriptorItem are not valid or the item
        /// index is out of bounds
        void UpdateDescriptorBuffer(DescriptorBuffer& buffer, const DescriptorItem& item, uint32_t itemIndex,
                                    DescriptorBufferType type, uint32_t setIndexInBuffer = 0,
                                    void* pMappedData = nullptr);

        /// @brief Binds the descriptor buffer to the command buffer
        /// @param buffers The descriptor buffers that will be bound
        /// @param cmdBuf The command buffer that will be used to record the bind
        void BindDescriptorBuffer(const std::vector<DescriptorBuffer>& buffers, vk::CommandBuffer cmdBuf);

        /// @brief Binds the descriptor set to the command buffer
        /// @param layout The pipeline layout that will be used to bind the descriptor set
        /// @param set The set where the descriptor set will be bound
        /// @param bufferIndex The index of the descriptor set in the descriptor buffer that will be bound.
        /// This is the index of the descriptor buffer that was given to BindDescriptorBuffer(...) function that will be
        /// bound
        /// @param offset The offset to where the buffer starts in the descriptor buffer that will be bound to the
        /// descriptor set If multiple descriptor sets are bound to the same descriptor buffer, this is the offset to
        /// the start of the descriptor set that will be bound
        /// @param cmdBuf The command buffer that will be used to record the bind
        /// @param bindPoint The bind point of the descriptor set, default is eRayTracingKHR
        void BindDescriptorSet(vk::PipelineLayout layout, uint32_t set, uint32_t bufferIndex, vk::DeviceSize offset,
                               vk::CommandBuffer cmdBuf,
                               vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eRayTracingKHR);

        /// @brief Binds the descriptor sets to the command buffer
        /// @param layout The pipeline layout that will be used to bind the descriptor set
        /// @param set The first set where the descriptor set will be bound
        /// @param bufferIndex The indices of the descriptor sets in the descriptor buffer that will be bound.
        /// This is the index of the descriptor buffer that was given to BindDescriptorBuffer(...) function that will be
        /// bound
        /// @param offset The offsets to where the buffer starts in the descriptor buffer that will be bound to the
        /// descriptor set If multiple descriptor sets are bound to the same descriptor buffer, this is the offset to
        /// the start of the descriptor set that will be bound
        /// @param cmdBuf The command buffer that will be used to record the bind
        /// @param bindPoint The bind point of the descriptor set, default is eRayTracingKHR
        void BindDescriptorSet(vk::PipelineLayout layout, uint32_t set, std::vector<uint32_t> bufferIndex,
                               std::vector<vk::DeviceSize> offset, // offset in the descriptor buffer, that is bound at
                                                                   // bufferIndex, to the descriptor set
                               vk::CommandBuffer cmdBuf,
                               vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eRayTracingKHR);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@ Shader Binding Table Functions @@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        /// @brief Gets the opaque handles for the shader records in the SBT buffer
        /// @param pipeline The pipeline that will be used to get the handles
        /// @param firstGroup The index of the first shader group in the pipeline
        /// @param groupCount The number of shader groups after firstGroup that will be used to get the handles
        /// @return The opaque handles for the shader records in the SBT buffer
        /// @note For Vulray's internal use, but can be used by the user doing custom SBT
        [[nodiscard]] std::vector<uint8_t> GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup,
                                                                  uint32_t groupCount);

        /// @brief Gets the opaque handles for the shader records in the SBT buffer
        /// @param pipeline The pipeline that will be used to get the handles
        /// @param firstGroup The index of the first shader group in the pipeline
        /// @param groupCount The number of shader groups after @c firstGroup that will be used to get the handles
        /// @param data The data that will be written to the handles
        /// @warning Segfault if the data pointer is not valid or the data size if out of bounds.
        /// The data must be minimum groupCount * RayTracingProperties::shaderGroupHandleSize bytes.
        /// @note For Vulray's internal use, but can be used by the user doing custom SBT
        void GetHandlesForSBTBuffer(vk::Pipeline pipeline, uint32_t firstGroup, uint32_t groupCount, void* data);

        /// @brief Writes data to a shader record in the SBT buffer
        /// @param sbtBuf The SBT buffer that will be written to
        /// @param group The shader group that will be written to
        /// @param groupIndex The index of the shader group that will be written to
        /// @param data The data that will be written to the shader record
        /// @param dataSize The size of the data in bytes that will be written to the shader record
        /// @param mappedData The pointer to the mapped data of the SBT buffer, if it is null, the buffer will be mapped
        /// and unmapped, default is nullptr
        /// @warning Segfault if any of the pointers are not valid or the data size if out of bounds
        void WriteToSBT(SBTBuffer sbtBuf, ShaderGroup group, uint32_t groupIndex, void* data, uint32_t dataSize,
                        void* mappedData = nullptr);

        /// @brief Creates a buffer for each shader type in the shader binding table
        /// @param pipeline The pipeline that will be used to create the SBT buffer
        /// @param sbt The information about the shader binding table, must contain the indices of the shader groups in
        /// the pipeline
        /// @return The SBT buffer object, which has buffers and vk::StridedDeviceAddressRegionKHR for each shader type
        /// in the shader binding table ready to be used in dispatching rays.
        [[nodiscard]] SBTBuffer CreateSBT(vk::Pipeline pipeline, const SBTInfo& sbt);

        /// @brief Rebuilds the SBT buffer with the new shader binding table info
        /// @param pipeline The pipeline that will be used to rebuild the SBT buffer
        /// @param buffer The SBT buffer that will be rebuilt
        /// @param sbt The information about the shader binding table, must contain the indices of the shader groups in
        /// the pipeline
        /// @return True if the SBT buffer was rebuilt successfully, false otherwise, because the SBT buffer is not big
        /// enough to fit all the shaders in the shader binding table info, then the user should call CreateSBT(...) to
        /// create a new SBT buffer and reserve more space for the shaders.
        /// @note This function doesn't reallocate the SBT buffer, it just rewrites the new opaque handles to the
        /// buffer. So you don't have to WriteToSBT(...) again after rebuilding the SBT buffer.
        bool RebuildSBT(vk::Pipeline pipeline, SBTBuffer& buffer, const SBTInfo& sbt);

        /// @brief Copies the whole SBT from a buffer to another, including the opaque handles.
        /// @param dst The SBT buffer that will be copied to
        /// @param src The SBT buffer that will be copied from
        /// @note dst must have the same or bigger size than src for all the SBT buffers.
        /// @example SBT too small, so create a new SBT buffer with bigger size and copy the old SBT to the new one.
        /// Then call RebuildSBT(...) to rewrite the opaque handles to the new SBT buffer, because SBT won't function
        /// with the old opaque handles. You would want to do this, because it copies the shader records, so you don't
        /// have to call WriteToSBT(...) again for even the old shader records.
        void CopySBT(SBTBuffer& src, SBTBuffer& dst);

        /// @brief Checks if the shaders can fit in the SBT buffer
        /// @param buffer The SBT buffer that will be checked
        /// @param sbtInfo The shader binding table info with the shader info that will be checked
        /// @return True if the SBT buffer has room to encapsulate the all the shaders in the shader binding table info,
        /// false otherwise
        bool CanSBTFitShaders(SBTBuffer& buffer, const SBTInfo& sbtInfo);

        /// @brief Destroys the SBT buffer
        /// @param buffer The SBT buffer that will be destroyed
        void DestroySBTBuffer(SBTBuffer& buffer);

        /// @brief Dispatches the rays
        /// @param rtPipeline The ray tracing pipeline that will be used to dispatch the rays
        /// @param buffer The SBT buffer that contains the shader records
        /// @param width The width of the image that will be used to dispatch the rays
        /// @param height The height of the image that will be used to dispatch the rays
        /// @param depth The depth of the image that will be used to dispatch the rays, default is 1
        /// @param cmdBuf The command buffer that will be used to record the dispatch
        void DispatchRays(const vk::Pipeline rtPipeline, const SBTBuffer& buffer, uint32_t width, uint32_t height,
                          uint32_t depth, vk::CommandBuffer cmdBuf);

        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@ Denoiser Functions @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifdef VULRAY_BUILD_DENOISERS

        /// @brief Creates a denoiser of type T
        /// @tparam T The type of the denoiser that will be created - Found in Denoisers namespace. will fail to compile
        /// if T is not a denoiser
        template <typename T,
                  typename std::enable_if<std::is_base_of<Denoise::DenoiserInterface, T>::value, int>::type = 0>
        vr::Denoiser CreateDenoiser(const Denoise::DenoiserSettings& settings)
        {
            return std::make_unique<T>(this, settings);
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
        bool mUserSuppliedAllocator = false;

        VmaPool mCurrentPool = nullptr;
    };

} // namespace vr
