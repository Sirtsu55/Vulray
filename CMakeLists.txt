# CMakeList.txt : CMake project for Vulray, include source and define
# project specific logic here.
#



cmake_minimum_required (VERSION 3.22)

project ("Vulray")

find_package(Vulkan 1.2 REQUIRED)

option(VULRAY_BUILD_VULKAN_BUILDER "Build the Vulkan Builder to quickstart Vulkan initialization" ON)
option(VULRAY_BUILD_DENOISER "Build the NRD denoiser" ON)

# Add third party libraries
if(VULRAY_BUILD_VULKAN_BUILDER)
	add_subdirectory("${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/")
endif()
if(VULRAY_BUILD_DENOISER)
	set(NRD_STATIC_LIBRARY ON)
	set(NRD_USE_PRECOMPILED_SHADERS ON)
	add_subdirectory("${PROJECT_SOURCE_DIR}/Vendor/RayTracingDenoiser/")
endif()


file(GLOB VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/*.cpp")

if(VULRAY_BUILD_VULKAN_BUILDER)
	file(GLOB VULKAN_BUILDER_SRC "${PROJECT_SOURCE_DIR}/Source/VulkanBuilder/*.cpp")
	list(APPEND VULRAY_SRC_FILES ${VULKAN_BUILDER_SRC})
endif()
if(VULRAY_BUILD_DENOISER)
	file(GLOB DENOISER_SRC "${VULRAY_SRC_FILES} ${PROJECT_SOURCE_DIR}/Source/Denoiser/*.cpp")
	list(APPEND VULRAY_SRC_FILES ${DENOISER_SRC})
endif()


# TODO: Another option to build as a shared library
add_library ("Vulray" STATIC ${VULRAY_SRC_FILES})

# Include Directories

target_precompile_headers("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Include/Vulray/Vulray.h")

if(VULRAY_BUILD_DENOISER)
	target_include_directories("Vulray" PUBLIC "${PROJECT_SOURCE_DIR}/Vendor/RayTracingDenoiser/Include/")
endif()

target_include_directories("Vulray" PUBLIC  "${PROJECT_SOURCE_DIR}/Include/")
target_include_directories("Vulray" PUBLIC  "${PROJECT_SOURCE_DIR}/Vendor/VulkanMemoryAllocator/include/")
target_include_directories("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/src/")
target_include_directories("Vulray" PRIVATE "${Vulkan_INCLUDE_DIRS}")

# Link Libraries
if(VULRAY_BUILD_DENOISER)
	target_link_libraries("Vulray" PUBLIC "NRD")
endif()
target_link_libraries("Vulray" PRIVATE "vk-bootstrap")
target_link_libraries("Vulray" PRIVATE ${Vulkan_LIBRARIES})

set_property(TARGET "Vulray" PROPERTY CXX_STANDARD 20)
