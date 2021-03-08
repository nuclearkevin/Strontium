# SciRender
A rendering engine / graphics sandbox I'm working on using OpenGL. I started working on this
project to learn computer graphics and GPU parallelism. Currently a WIP with
very few features and no planned features.

This project is not intended to be a high performance rendering engine, but rather a sandbox for experimenting with various techniques commonly used for realtime and photorealistic rendering. Additionally, I would like to use this as a testbed for in-situ neutronics simulations (first using the diffusion approximation and than a Monte-Carlo approach). 

I also hope to eventually use a stripped down version of this application as the frontend for
[FinDPyNe](https://github.com/ksawatzky777/FinDPyNE), replacing the PyGame
GUI that it currently uses.

If you want to download and run the skeleton of an application that is SciRender,
follow the steps below:
1. Download and install GL, GLU, GLEW, GLFW and GLM.
2. Download and install make (the build system SciRender uses) and git
(version control).
3. Run the following commands:
```bash
git clone https://github.com/ksawatzky777/SciRender.git
cd SciRender
make
./Application
```

I can guarantee that this application compiles and runs successfully on Linux, attempt to compile+run on other operating systems at your own peril!
~~I should really include the OpenGL dependancies with this application now that I think about it.~~
