# Vulray - Raytracing Simplified

<img src=https://user-images.githubusercontent.com/65868911/229463845-60a57bbd-4e80-4f1b-930e-e90a0569b7e1.jpg width="auto" height="auto">

## Introduction
Vulray is a library to simplify Vulkan Raytracing.
This library is meant for people wanting to dip their hands into Vulkan Raytracing without any worries of writing boilerplate code for Acceleration Structure, SBT, Descriptors, Pipelines, etc. 
Using the library does precede some basic understanding of Vulkan objects. 
## Features
- Creating Vulkan Device & Instance  (~20LoC) ✅
- Bottom Level Acceleration Build/Update: ✅
- Top Level Acceleration Build/Update: ✅
- BLAS Compaction: ✅
- Ray Tracing Pipeline Creation: ✅
- Pipeline Libraries: ✅
- SBT Creation/Update: ✅
- Descriptor Set Creation/Update (Descriptor Buffer Extension): ✅
- Buffer/Image Creation: ✅

## Getting Started ...

VulraySamples has examples of using the Vulray Library with explanations: https://github.com/Sirtsu55/VulraySamples

It also has a RTIOW - like tutorial: https://sirtsu55.github.io/blog/RayTracing/VulrayInOneWeekend/Chapter0.html
## Requirements
- Vulkan SDK 1.3
- Vulkan Ray Tracing capable device. [Check GPU for support](https://vulkan.gpuinfo.org/listdevicescoverage.php?extension=VK_KHR_ray_tracing_pipeline&platform=all)
- Tested with CMake version 3.25
- A compiler with C++ 20 support 

## Integrating into projects
- Add as submodule or download the files
- Add ```add_subdirectory("{install_folder}/Vulray")``` to your ```CMakeLists.txt```
- Add Include Directory ```Vulray/Include/```
- Link with `Vulray` CMake Target
- Refer to [VulraySamples](https://github.com/Sirtsu55/VulraySamples
) if stuck

## Feature Request & Contributing
If you want a feature please open an Issue and I will try to add it. Denoiser suggestions or other ray tracing features  are welcome. Contributing via pull requests are welcome also.

## Flexible and Transparent? Yes
Vulray exposes native Vulkan handles to the user. Some steps of creating objects can be done by the user, as long as the appropriate fields are filled for upcoming steps. For example, user can chose to create an index buffer with their own code, but the Vulray's structure for a Buffer has to be filled by the user manually to continue using Vulray functions.
