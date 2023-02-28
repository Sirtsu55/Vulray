# Vulray - Raytracing Simplified

## Introduction
Vulray is a library to simplify Vulkan Raytracing.
This library is meant for people wanting to dip their hands into Vulkan Raytracing without any worries of writing boilerplate code for Acceleration Structure, SBT, Descriptors, Pipelines, etc.
Using the library does precede some basic understanding of Vulkan objects.

## Features
- Easily creating Vulkan Device & Instance (~20LoC) ✅
- Bottom Level Acceleration Build: ✅
- Bottom Level Acceleration Update: ❌ **Upcoming Feature**
- Top Level Acceleration Build/Update: ✅
- SBT Creation/Update: ✅
- Descriptor Set Creation/Update: ✅
- Dynamic Descriptor Creation: ✅
- Ray Tracing Pipeline Creation: ✅
- Buffer/Image Creation: ✅
- Transparent Vulkan objects for more control: ✅

## Getting Started ...

VulraySamples has examples of using the Vulray Library with explanations: https://github.com/Sirtsu55/VulraySamples

## Requirements
- Vulkan SDK 1.2 or newer
- Vulkan Ray Tracing capable device. [Check GPU for support](https://vulkan.gpuinfo.org/listdevicescoverage.php?extension=VK_KHR_ray_tracing_pipeline&platform=all)
- Tested with CMake version 3.22 (can be older)
- A compiler with C++ 20 support 

## Integrating into projects
- Add as submodule or download the files
- Add ```add_subdirectory("{folder}/Vulray")``` to your ```CMakeLists.txt```
- Add Include Directory ```Vulray/Include/```
- Add ```{folder}/Vulray/``` as link directory and link with ```Vulray.lib```
- Refer to [VulraySamples](https://github.com/Sirtsu55/VulraySamples
) if stuck
  

## Flexible and Transparent? Yes
Vulray exposes native Vulkan handles to the user. Some steps of creating objects can be done by the user, as long as the appropriate fields are filled for upcoming steps. For example, user can chose to create an index buffer with their own code, but the Vulray's structure for a Buffer has to be filled by the user manually to continue using Vulray functions.
