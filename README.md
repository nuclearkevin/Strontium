<h1 align="center"><img align="center" src="https://github.com/ksawatzky777/Strontium/blob/main/media/strontium.png" width="128px"/>

Strontium

</h1>
<p align="center"> Yet another open-source C++17 game engine using OpenGL. </p>
This project started while I was taking a class on intermediate computer graphics, but has spiraled out of control to become something far more than just a practice repo.

![sponza](https://github.com/ksawatzky777/SciRender/blob/main/media/sponza.png)

![dancing](https://github.com/ksawatzky777/SciRender/blob/main/media/dancing.gif)

### <h1 align="center">Current Features</h1>
- Graphics:
  - A deferred 3D physically based rendering using the Cook-Torrance BRDF.
  - Image-based ambient lighting using HDR environment maps.
  - Directional sky lighting.
  - Point lights.
  - Cascaded exponentially warped shadow maps utilizing parallel splits.
  - Physically-based materials using a metallic workflow.
  - HDR rendering.
  - Skeletal animation.
- Other:
  - Custom scene serialization and loading.
  - Custom material serialization and loading.
  - A syncable prefab system for rapid scene prototyping.
  - Live shader editing and reloading.
  - Multithreaded asset loading for many supported model formats (using Assimp) and images.
  - A modern editor (with docking) designed with rapid prototyping in mind.
  - And lots more!

### <h1 align="center">Planned Features</h1>
- Graphics:
  - 2D rendering for particles, sprites and billboards.
  - Spot lights.
  - Transparent support.
  - Volumetric directional lighting.
  - Screen space ambient occlusion.
  - Screen space reflections.
  - Screen space voxel global illumination.
  - Additional shadow mapping options for user defined light sources.
- Physics using NVIDIA PhysX.
- C# Scripting using Mono:
  - Large portions of the internal engine exposed to user scripting.
  - Completely scriptable render pipeline.
- Other:
  - Asset caching and optimization.
  - Automatic material loading for common file varieties.
  - Multi-OS support.
  - DX11 support with live API switching.

## Building
Strontium builds using GNU make. GLFW and Assimp require CMake, while GLFW also requires the `xorg-dev` package. Ensure you have these dependencies installed before building Strontium. The steps below are currently outdated as this project is moving towards a more formal buildsystem using CMake.
```bash
# Clone the project and initialize submodules.
git clone https://github.com/ksawatzky777/SciRender.git --recursive
# Build glfw.
cd SciRender/vendor/glfw
cmake .
make
# Build Assimp.
cd ../assimp
cmake CMakeLists.txt
cmake --build .
# Build SciRender
cd ../..
make
```
Finally, you can run the application with the following bash command:
```bash
./Application
```
Currently only Ubuntu builds are supported, although care has been taken to ensure that this project is as platform agnostic as possible. Windows builds will be supported in the future as this project matures.

## Credits
A massive thanks goes out to [Yan Chernikov](https://github.com/TheCherno) and his several Youtube series on
[OpenGL and game engine design](https://www.youtube.com/user/TheChernoProject). Without them,
this pet project would not have gotten nearly as far as it is today!
