
#pragma once


//STL 
#include <iostream>
#include <vector>
#include <memory>
#include <set>



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

//Logging Colors
#define VULRAY_LOG_WHITE(log) VULRAY_LOG_STREAM		<< "\033[0m [Vulray]: "  << log << VULRAY_LOG_END << "\n"
#define VULRAY_LOG_RED(log) VULRAY_LOG_STREAM		<< "\x1B[31m [Vulray]: " << log << VULRAY_LOG_END << "\n"
#define VULRAY_LOG_GREEN(log) VULRAY_LOG_STREAM		<< "\x1B[32m [Vulray]: " << log << VULRAY_LOG_END << "\n"
#define VULRAY_LOG_YELLOW(log) VULRAY_LOG_STREAM	<< "\x1B[33m [Vulray]: " << log << VULRAY_LOG_END << "\n"
#define VULRAY_LOG_PURPLE(log) VULRAY_LOG_STREAM	<< "\x1B[35m [Vulray]: " << log << VULRAY_LOG_END << "\n"
#define VULRAY_LOG_GRAY(log) VULRAY_LOG_STREAM		<< "\x1B[90m [Vulray]: " << log << VULRAY_LOG_END << "\n"






