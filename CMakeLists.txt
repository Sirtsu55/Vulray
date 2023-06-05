# CMakeList.txt : CMake project for Vulray, include source and define
# project specific logic here.
#



cmake_minimum_required (VERSION 3.22)

project ("Vulray")

find_package(Vulkan 1.3 REQUIRED)

#add vk-bootstrap
add_subdirectory("${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/")

#add Denoiser library
set(NRD_STATIC_LIBRARY ON) # link to static library
add_subdirectory("${PROJECT_SOURCE_DIR}/Vendor/RayTracingDenoiser/")

file(GLOB_RECURSE VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/*.cpp")

if(VULRAY_BUILD_SHARED)
	add_library("Vulray" SHARED ${VULRAY_SRC_FILES})
	add_compile_definitions(TARGET "Vulray" VULRAY_BUILD_SHARED)
else()
	add_library("Vulray" STATIC ${VULRAY_SRC_FILES})
endif()

target_include_directories("Vulray" PUBLIC 
	"${Vulkan_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/Vendor/RayTracingDenoiser/Include/"
	"${PROJECT_SOURCE_DIR}/Vendor/VulkanMemoryAllocator/include/"
	"${PROJECT_SOURCE_DIR}/Include/"
)

target_include_directories("Vulray" PRIVATE 
	"${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/src/"
)

target_link_libraries("Vulray" PRIVATE "vk-bootstrap" ${Vulkan_LIBRARIES} "NRD")

target_precompile_headers("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Include/Vulray/Vulray.h")

set_property(TARGET "Vulray" PROPERTY CXX_STANDARD 20)
