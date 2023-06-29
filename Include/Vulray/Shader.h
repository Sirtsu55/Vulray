#pragma once

namespace vr
{
    struct Shader
    {
        /// @brief Shader module handle
        vk::ShaderModule Module = nullptr;

        /// @brief If there are multiple entry points in the shader, this is the entry point that will be used, Default
        /// is "main"
        const char* EntryPoint = "main";
    };

} // namespace vr
