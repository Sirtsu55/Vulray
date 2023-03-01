#pragma once


//STL 
#include <iostream>
#include <vector>
#include <memory>
#include <set>
#include <format>



#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "Vulray/AccelStruct.h"
#include "Vulray/Buffer.h"
#include "Vulray/Shader.h"
#include "Vulray/SBT.h"
#include "Vulray/Descriptors.h"
#include "Vulray/VulrayDevice.h"
#include "Vulray/VulkanBuilder.h"


#define VULRAY_LOG_STREAM std::cerr
#define VULRAY_LOG_END "\033[0m"

namespace vr
{
    enum class MessageType { Verbose, Info, Warning, Error};

    //Vulray Log Callback
    typedef void(*VulrayLogCallback)(const std::string&, MessageType);

    // Logging Callback variable for vulray, if set, vulray will call this function instead of printing to console
    // if specified, Vulkan validation layer messages will be relayed to this callback if a 
    // VulkanBuilder::DebugCallback is not specified when building the vulkan instance using Vulray::VulkanBuilder
    extern VulrayLogCallback LogCallback;

}
//Logging Colors
#define VULRAY_LOG_VERBOSE(fmt, ...) if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM  << "\x1B[90m [Vulray]: " << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Verbose);  }
#define VULRAY_LOG_INFO(fmt, ...)    if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\033[0m [Vulray]: "  << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Info);     }
#define VULRAY_LOG_WARNING(fmt, ...) if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\x1B[33m [Vulray]: " << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Warning);  }
#define VULRAY_LOG_ERROR(fmt, ...)   if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\x1B[31m [Vulray]: " << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Error);    }






