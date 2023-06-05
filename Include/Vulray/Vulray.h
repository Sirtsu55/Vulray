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


#define VULRAY_LOG_STREAM std::cerr
#define VULRAY_LOG_END "\033[0m"

namespace vr
{

    //Vulray Log Callback
    typedef void(*VulrayLogCallback)(const std::string&, MessageType);

    // Logging Callback variable for vulray, if set, vulray will call this function instead of printing to console
    // if specified, Vulkan validation layer messages will be relayed to this callback if a 
    // VulkanBuilder::DebugCallback is not specified when building the vulkan instance using Vulray::VulkanBuilder
    extern VulrayLogCallback LogCallback;

}

// FLOG means formatted log

#define VULRAY_FLOG_VERBOSE(fmt, ...) if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM << "\x1B[90m [Vulray]: "    << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Verbose);  }
#define VULRAY_FLOG_INFO(fmt, ...)    if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\033[0m  [Vulray]: "    << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Info);     }
#define VULRAY_FLOG_WARNING(fmt, ...) if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\x1B[33m [Vulray]: "    << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Warning);  }
#define VULRAY_FLOG_ERROR(fmt, ...)   if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\x1B[31m [Vulray]: "    << std::format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(std::format(fmt, __VA_ARGS__), vr::MessageType::Error);    }

#define VULRAY_LOG_VERBOSE(message) if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM  << "\x1B[90m [Vulray]: "    << message << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(message, vr::MessageType::Verbose);  }
#define VULRAY_LOG_INFO(message)    if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\033[0m  [Vulray]: "    << message << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(message, vr::MessageType::Info);     }
#define VULRAY_LOG_WARNING(message) if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\x1B[33m [Vulray]: "    << message << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(message, vr::MessageType::Warning);  }
#define VULRAY_LOG_ERROR(message)   if(vr::LogCallback == nullptr){ VULRAY_LOG_STREAM	<< "\x1B[31m [Vulray]: "    << message << VULRAY_LOG_END << "\n"; } else {vr::LogCallback(message, vr::MessageType::Error);    }










