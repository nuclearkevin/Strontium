# SciRender
## About this project
Yet another open-source rendering engine / graphics sandbox using OpenGL. This project started while I was taking a class on intermediate computer graphics (as I intended to use it to implement various algorithms learned in class), but has spiraled out of control to become something far more than just a practice repo.

This project is not intended to be a high performance rendering engine, but rather a sandbox for experimenting with various techniques commonly used for realtime and photorealistic rendering. It has the following tentative list of planned features:
- In-app scene selection, modification and serialization.
- Scene and model loading for common file formats (using Assimp).
- A deferred (physically based) 3D renderer with light volumes.
  - Cascaded softshadows for point lights, spotlights and uniform lights.
  - Screen space ambient occlusion.
  - Screen space reflections (single bounce).
  - Transparent support (using model sorting).
- A deferred 2D renderer for sprites, billboards, etc.
- A global illumination engine for rendering a static image of the scene.
- Realistic physics simulations derived from first principles (possibly as extra modules).
  - This will likely focus on the physics commonly found in nuclear reactors (considering my background).
- Vulkan support.

## Building SciRender
SciRender builds using GNU make. Additionally, GLFW requires CMake and the `xorg-dev` packages. Assimp also requires CMake. Ensure you have these dependencies installed before building SciRender, which can be done with the steps below:
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
As of right now, this project only builds successfully on Linux (I develope and test on Debian-Ubuntu). I aim to eventually support Windows builds using Visual Studios, but thats a goal for the future. Linux will be the only supported build for now.

## Credits
A massive thanks goes out to [Yan Chernikov](https://github.com/TheCherno) and his several Youtube series on
[OpenGL and game engine design](https://www.youtube.com/user/TheChernoProject). Without them,
this pet project would not have gotten nearly as far as it is today!
