<h1 align="center"><img align="center" src="https://github.com/ksawatzky777/Strontium/blob/main/media/strontium.png" width="128px"/>

Strontium

</h1>
<p align="center"> Yet another open-source C++17 game engine using OpenGL. </p>
This project started while I was taking a class on intermediate computer graphics, but has spiraled out of control to become something far more than just a practice repo.

![test](https://github.com/ksawatzky777/Strontium/blob/main/media/testscene.png)

![sponza](https://github.com/ksawatzky777/Strontium/blob/main/media/sponza.png)

### <h1 align="center">Current Features</h1>
- Graphics:
  - A deferred 3D physically based renderer using the Cook-Torrance BRDF.
  - A dynamic skybox system for fast, fully integrated atmospheric lighting.
  - Dynamic image-based lighting for the skyboxes mentioned above.
  - Directional lights with support for a single light with cascaded shadows.
    - A variety of shadow quality options which include percentage-closer soft shadows.
  - Many point and spot lights through deferred tiled light culling.
  - Rectangular analytical area lights (LTCs).
  - Unified volumetrics for fog and haze.
    - Supports bounding box fog volumes, spherical fog volumes, depth-based fog, and exponential height fog.
  - Horizon-based ambient occlusion.
  - Physically-based materials using a metallic workflow.
  - HDR rendering.
  - Skeletal and rigged animation.
- Physics
  - A robust implementation of Jolt Physics.
  - Several colliders (sphere, box, cylinder, capsule).
  - Collision callbacks for:
    - A contact being added between two shapes.
    - A contact persisting between two shapes.
    - A contact being removed between two shapes.
- Other:
  - Custom scene serialization and loading.
  - Custom material serialization and loading.
  - A syncable prefab system for rapid scene prototyping. Currently a WIP.
  - Multithreaded asset loading for many supported model formats (using Assimp) and images.
  - A modern editor (with docking) designed with rapid prototyping in mind.
  - And lots more!

### <h1 align="center">Planned Features</h1>
- Graphics:
  - 2D rendering for particles, sprites and billboards.
  - Transparent object support.
  - Screen-space glossy reflections.
  - Screen-space voxel global illumination.
  - Point and spot light shadow maps.
- C# Scripting.

## Building
Strontium builds using CMake, so ensure that you have CMake installed before you attempt to build the project. As Strontium is fully platform agnostic, both Windows and Linux builds are possible (although the Linux buildsystem is WIP).
Start by recursively cloning the repo using git:
```bash
git clone https://github.com/ksawatzky777/Strontium.git --recursive
```
Then, create a build directory in the repo source
```bash
cd Strontium
mkdir build
```
Afterwards, you can run CMake from the build directory and specify that source directory is in Strontium:
```bash
cd build
cmake ../
```
If you're on Windows, you can open the StrontiumEngine.sln solution file. Set the startup project to StrontiumEditor and build with Visual Studios.

## Dependencies
- Assimp
- EnTT
- Glad
- GLFW
- GLM
- Dear ImGui
- ImGui-Addons (notably for the cross-platform Dear ImGui-based file browser)
- ImGuizmo
- stb_image and std_image_write
- yaml-cpp
- Jolt Physics
