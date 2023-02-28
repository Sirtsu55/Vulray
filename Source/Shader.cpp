#include "Vulray/Shader.h"

namespace vr
{
    Shader VulrayDevice::CreateShaderFromSPV(ShaderCreateInfo& info)
    {
        Shader outShader = {};
        if(info.SPIRVCode.empty())
        {
            VULRAY_LOG_RED("ShaderCreateInfo must have SPIRV code");
            return outShader; // return empty shader, because no shader was created
        }

        outShader.Module = CreateShaderModule(info.SPIRVCode);
        outShader.Stage = info.Stage;

        return outShader;
    }

    void VulrayDevice::DestroyShader(Shader& shader)
    {
        mDevice.destroyShaderModule(shader.Module);
    }


    vk::ShaderModule VulrayDevice::CreateShaderModule(const std::vector<uint32_t>& spvCode)
    {
		//create shader module
		auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
			.setCodeSize(spvCode.size() * sizeof(uint32_t))
			.setPCode(spvCode.data());
        
		auto shaderModule = mDevice.createShaderModule(shaderModuleCreateInfo);
        
        return shaderModule;
    }


}


