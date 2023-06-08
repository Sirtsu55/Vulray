# CMakeList.txt : CMake project for Vulray, include source and define
# project specific logic here.
#



cmake_minimum_required(VERSION 3.22)

project("Vulray")

find_package(Vulkan 1.3 REQUIRED)


# -------------- Options --------------

option(VULRAY_BUILD_DENOISERS "Build denoisers" ON)
option(VULRAY_BUILD_VULKAN_BUILDER "Build bootsraps for easy Vulkan Initialization" ON)

# -------------- Dependencies --------------

#add vk-bootstrap
if(VULRAY_BUILD_VULKAN_BUILDER)
	add_subdirectory("${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/")
endif()

#add Denoiser library
if(VULRAY_BUILD_DENOISERS)
	set(NRD_STATIC_LIBRARY ON) # link to static library
	add_subdirectory("${PROJECT_SOURCE_DIR}/Vendor/RayTracingDenoiser/")
endif()
# -------------- Source Files --------------

# Add all source files for minimal build
file(GLOB VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/*.cpp")

# Add files depending on options
if(VULRAY_BUILD_VULKAN_BUILDER)
	list(APPEND VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/VulkanBuilder/VulkanBuilder.cpp")
endif()
if(VULRAY_BUILD_DENOISERS)
	list(APPEND VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/Denoiser/Denoiser.cpp")
endif()

add_library("Vulray" STATIC ${VULRAY_SRC_FILES})


# -------------- Header File Options --------------

target_precompile_headers("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Include/Vulray/Vulray.h")

target_include_directories("Vulray" PUBLIC 
	"${Vulkan_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/Vendor/VulkanMemoryAllocator/include/"
	"${PROJECT_SOURCE_DIR}/Include/"
	)
if(VULRAY_BUILD_DENOISERS)
	target_include_directories("Vulray" PUBLIC "${PROJECT_SOURCE_DIR}/Vendor/RayTracingDenoiser/Include/")
endif()
if(VULRAY_BUILD_VULKAN_BUILDER)
	target_include_directories("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/include/")
endif()

# -------------- Link Libraries --------------

if(VULRAY_BUILD_DENOISERS)
	target_link_libraries("Vulray" PRIVATE "NRD")
endif()
if(VULRAY_BUILD_VULKAN_BUILDER)
	target_link_libraries("Vulray" PRIVATE "vk-bootstrap")
endif()

target_link_libraries("Vulray" PRIVATE ${Vulkan_LIBRARIES})


set_property(TARGET "Vulray" PROPERTY CXX_STANDARD 20)
