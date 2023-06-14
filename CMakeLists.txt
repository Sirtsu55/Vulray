
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

# -------------- Source Files --------------

# Add all source files for minimal build
file(GLOB VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/*.cpp")

# Add files depending on options
if(VULRAY_BUILD_VULKAN_BUILDER)
	list(APPEND VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/VulkanBuilder/VulkanBuilder.cpp")
endif()
if(VULRAY_BUILD_DENOISERS)
	file(GLOB DENOISER_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/Denoisers/*.cpp")
	list(APPEND VULRAY_SRC_FILES ${DENOISER_SRC_FILES})
	message ("Denoisers: ${DENOISER_SRC_FILES}")
endif()

add_library("Vulray" STATIC ${VULRAY_SRC_FILES})

if(VULRAY_BUILD_DENOISERS)
	target_compile_definitions("Vulray" PUBLIC "VULRAY_BUILD_DENOISERS")
endif()

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

if(VULRAY_BUILD_VULKAN_BUILDER)
	target_link_libraries("Vulray" PRIVATE "vk-bootstrap")
endif()

target_link_libraries("Vulray" PRIVATE ${Vulkan_LIBRARIES})


set_property(TARGET "Vulray" PROPERTY CXX_STANDARD 20)
