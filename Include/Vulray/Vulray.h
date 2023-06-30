#pragma once

#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "Vulray/AccelStruct.h"
#include "Vulray/Buffer.h"
#include "Vulray/Descriptors.h"
#include "Vulray/SBT.h"
#include "Vulray/Shader.h"
#include "Vulray/VulrayDevice.h"

#define VULRAY_LOG_STREAM std::cerr
#define VULRAY_LOG_END "\033[0m"

namespace vr
{

    // Vulray Log Callback
    typedef void (*VulrayLogCallback)(const std::string&, MessageType);

    // Logging Callback variable for vulray, if set, vulray will call this function instead of printing to console
    // if specified, Vulkan validation layer messages will be relayed to this callback if a
    // VulkanBuilder::DebugCallback is not specified when building the vulkan instance using Vulray::VulkanBuilder
    extern VulrayLogCallback LogCallback;

    // Stackoverflow: https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
    // Guard against multiple definitions
#ifndef _VULRAY_LOG_DEF
#define _VULRAY_LOG_DEF
    template <typename... Args> constexpr std::string _string_format(const std::string& format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
        if (size_s <= 0)
        {
            return "LOGGING ERROR";
        }
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }
#endif
} // namespace vr

// FLOG means formatted log
#define VULRAY_FLOG_VERBOSE(fmt, ...)                                                                                  \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\x1B[90m [Vulray]: " << vr::_string_format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n";  \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(vr::_string_format(fmt, __VA_ARGS__), vr::MessageType::Verbose);                               \
    }
#define VULRAY_FLOG_INFO(fmt, ...)                                                                                     \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\033[0m  [Vulray]: " << vr::_string_format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n";  \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(vr::_string_format(fmt, __VA_ARGS__), vr::MessageType::Info);                                  \
    }
#define VULRAY_FLOG_WARNING(fmt, ...)                                                                                  \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\x1B[33m [Vulray]: " << vr::_string_format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n";  \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(vr::_string_format(fmt, __VA_ARGS__), vr::MessageType::Warning);                               \
    }
#define VULRAY_FLOG_ERROR(fmt, ...)                                                                                    \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\x1B[31m [Vulray]: " << vr::_string_format(fmt, __VA_ARGS__) << VULRAY_LOG_END << "\n";  \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(vr::_string_format(fmt, __VA_ARGS__), vr::MessageType::Error);                                 \
    }

#define VULRAY_LOG_VERBOSE(message)                                                                                    \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\x1B[90m [Vulray]: " << message << VULRAY_LOG_END << "\n";                               \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(message, vr::MessageType::Verbose);                                                            \
    }
#define VULRAY_LOG_INFO(message)                                                                                       \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\033[0m  [Vulray]: " << message << VULRAY_LOG_END << "\n";                               \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(message, vr::MessageType::Info);                                                               \
    }
#define VULRAY_LOG_WARNING(message)                                                                                    \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\x1B[33m [Vulray]: " << message << VULRAY_LOG_END << "\n";                               \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(message, vr::MessageType::Warning);                                                            \
    }
#define VULRAY_LOG_ERROR(message)                                                                                      \
    if (vr::LogCallback == nullptr)                                                                                    \
    {                                                                                                                  \
        VULRAY_LOG_STREAM << "\x1B[31m [Vulray]: " << message << VULRAY_LOG_END << "\n";                               \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        vr::LogCallback(message, vr::MessageType::Error);                                                              \
    }
