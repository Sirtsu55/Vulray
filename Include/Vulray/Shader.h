#pragma once

namespace vr
{
    
    struct ShaderCreateInfo
    {
        std::vector<uint32_t> SPIRVCode;
    };

    struct Shader
    {
        vk::ShaderModule Module = nullptr;
        vk::ShaderStageFlagBits Stage = vk::ShaderStageFlagBits::eAll;
        const char* EntryPoint = "main"; //
    };  

} 
