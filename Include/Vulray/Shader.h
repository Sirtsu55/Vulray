#pragma once

namespace vr
{
    /// @brief Structure that is used to create a shader module
    struct ShaderCreateInfo
    {
        /// @brief The SPIRV code that will be used to create the shader module
        std::vector<uint32_t> SPIRVCode;
    };

    struct Shader
    {
        /// @brief Shader module handle
        vk::ShaderModule Module = nullptr;

        /// @brief If there are multiple entry points in the shader, this is the entry point that will be used, Default is "main"
        const char* EntryPoint = "main";
    };  

} 
