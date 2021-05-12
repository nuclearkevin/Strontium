# SciRender
A rendering engine / graphics sandbox I'm working on using OpenGL. I started working on this
project to learn computer graphics and GPU parallelism. Currently a WIP with
very few features and no planned features.

This project is not intended to be a high performance rendering engine, but rather a sandbox for experimenting with various techniques commonly used for realtime and photorealistic rendering.

A massive thanks goes out to [Yan Chernikov](https://github.com/TheCherno) and his several Youtube series on
[OpenGL and game engine design](https://www.youtube.com/user/TheChernoProject). Without them,
this pet project would not have gotten nearly as far as it is today!

SciRender builds using GNU make. If you want to download and run the skeleton of an application that is SciRender,
follow the steps below:
1. Download and install GLFW.
2. Run the following commands:
```bash
git clone https://github.com/ksawatzky777/SciRender.git --recursive
git submodule init
cd SciRender
make
./Application
```

I can guarantee that this application compiles and runs successfully on Linux (Ubuntu).
Attempt to compile and run this application on other operating systems at your own peril!
