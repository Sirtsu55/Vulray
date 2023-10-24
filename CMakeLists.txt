
cmake_minimum_required(VERSION 3.25)

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

if(VULRAY_BUILD_VULKAN_BUILDER)
	target_include_directories("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Vendor/vk-bootstrap/include/")
endif()
if(VULRAY_BUILD_DENOISERS)
	target_include_directories("Vulray" PRIVATE "${PROJECT_SOURCE_DIR}/Shaders/Bin/") # For denoiser shaders
endif()

# -------------- Link Libraries --------------

if(VULRAY_BUILD_VULKAN_BUILDER)
	target_link_libraries("Vulray" PRIVATE "vk-bootstrap")
endif()

target_link_libraries("Vulray" PRIVATE ${Vulkan_LIBRARIES})


set_property(TARGET "Vulray" PROPERTY CXX_STANDARD 20)

# -------------- Compile Denoiser Shaders --------------

if(VULRAY_BUILD_DENOISERS)

	# Create a custom target for compiling the denoiser shaders
	add_custom_target("VulrayDenoiserShaders")

	file(GLOB DENOISER_SHADER_FILES "${PROJECT_SOURCE_DIR}/Shaders/*.hlsl")

	# Make sure the output directory exists

	file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/Shaders/Bin/")

	foreach(SHADER_FILE ${DENOISER_SHADER_FILES})

		get_filename_component(SHADER_FILENAME "${SHADER_FILE}" NAME_WE)

		set(SHADER_OUTPUT "${PROJECT_SOURCE_DIR}/Shaders/Bin/${SHADER_FILENAME}.spv.h")

		add_custom_command(TARGET "VulrayDenoiserShaders"
			COMMENT "Compiling shader ${SHADER_FILE}"
			COMMAND ${Vulkan_dxc_EXECUTABLE}
			-T cs_6_5
			-E ${SHADER_FILENAME}_main
			-spirv
			-O3
			-fspv-target-env=vulkan1.3
			-Qstrip_debug
			-Fh "${SHADER_OUTPUT}"
			"${SHADER_FILE}"
			)
	endforeach()

	# Make sure the shaders are compiled before building the library
	add_dependencies("Vulray" "VulrayDenoiserShaders")

endif()
