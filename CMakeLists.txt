# CMakeList.txt : CMake project for Vulray, include source and define
# project specific logic here.
#



cmake_minimum_required (VERSION 3.22)

project ("Vulray")

find_package(Vulkan 1.2 REQUIRED)

#add vk-bootstrap
add_subdirectory("${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/")



file(GLOB_RECURSE VULRAY_SRC_FILES "${PROJECT_SOURCE_DIR}/Source/*.cpp")

if(VULRAY_BUILD_SHARED)
	add_library("Vulray" SHARED ${VULRAY_SRC_FILES})
	add_compile_definitions(TARGET "Vulray" VULRAY_BUILD_SHARED)
else()
	add_library("Vulray" STATIC ${VULRAY_SRC_FILES})
endif()
target_include_directories("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/src/")
target_include_directories("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Vendor/VulkanMemoryAllocator/include/")
target_include_directories("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Include/")
target_include_directories("Vulray" PRIVATE "${Vulkan_INCLUDE_DIRS}")

target_link_libraries("Vulray" PRIVATE "vk-bootstrap")
target_link_libraries("Vulray" PRIVATE ${Vulkan_LIBRARIES})

target_precompile_headers("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Include/Vulray/Vulray.h")


set_property(TARGET "Vulray" PROPERTY CXX_STANDARD 20)
