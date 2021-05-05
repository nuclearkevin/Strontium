// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Core/Logs.h"
#include "Graphics/Meshes.h"
#include "Graphics/Shaders.h"
#include "Graphics/Renderer.h"
#include "Graphics/Camera.h"
#include "Graphics/Buffers.h"
#include "Graphics/Textures.h"

// STL includes.
#include <unordered_map>

namespace SciRenderer
{
  class EnvironmentMap
  {
  public:
    EnvironmentMap(const char* vertPath, const char* fragPath,
                   const char* cubeMeshPath);
    ~EnvironmentMap();

    // Load 6 textures from a file to generate a cubemap.
    void loadCubeMap(const std::vector<std::string> &filenames,
                     const MapType &type);

    // Bind/unbind a specific cubemap.
    void bind(const MapType &type);
    void unbind();

    // Binds one of the environment map PBR textures to a point.
    void bindToPoint(const MapType &type, GLuint bindPoint);

    // Draw the skybox.
    void draw(Camera* camera);

    // Generate the diffuse irradiance map.
    void precomputeIrradiance(GLuint width, GLuint height);

    // Generate the specular map components (pre-filter and BRDF integration map).
    void precomputeSpecular(GLuint width, GLuint height);
  protected:
    CubeMap*   skybox;
    CubeMap*   irradiance;
    CubeMap*   specPrefilter;
    Texture2D* brdfIntMap;

    Shader*  cubeShader;

    Mesh*    cube;

    bool     hasSkybox;
    bool     hasIrradiance;
  };
}
