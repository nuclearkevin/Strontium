// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/Logs.h"
#include "Graphics/Meshes.h"
#include "Graphics/Shaders.h"
#include "Graphics/Renderer.h"
#include "Graphics/Camera.h"
#include "Graphics/Buffers.h"
#include "Graphics/Textures.h"

namespace SciRenderer
{
  enum class MapType { SKYBOX, IRRADIANCE, PREFILTER, INTEGRATION };

  class EnvironmentMap
  {
  public:
    EnvironmentMap(const char* vertPath, const char* fragPath,
                   const char* cubeMeshPath);
    ~EnvironmentMap();

    // Load 6 textures from a file to generate a cubemap.
    void loadCubeMap(const std::vector<std::string> &filenames,
                     const TextureCubeMapParams &params = TextureCubeMapParams(),
                     const bool &isHDR = false);

    // Load a 2D equirectangular map, than convert it to a cubemap.
    // Assumes that the map is HDR by default.
    void loadEquirectangularMap(const std::string &filepath,
                                const Texture2DParams &params = Texture2DParams(),
                                const bool &isHDR = true,
                                const GLuint &width = 512,
                                const GLuint &height = 512);

    // Bind/unbind a specific cubemap.
    void bind(const MapType &type);
    void unbind();

    // Binds one of the environment map PBR textures to a point.
    void bindToPoint(const MapType &type, GLuint bindPoint);

    // Draw the skybox.
    void draw(Camera* camera);

    // Generate the diffuse irradiance map.
    void precomputeIrradiance(GLuint width, GLuint height, bool isHDR = false);

    // Generate the specular map components (pre-filter and BRDF integration map).
    void precomputeSpecular(GLuint width, GLuint height, bool isHDR = false);

    // Getters.
    inline GLfloat& getGamma() { return this->gamma; }
    inline glm::vec3& getExposure() { return this->exposure; }
    
  protected:
    CubeMap*   skybox;
    CubeMap*   irradiance;
    CubeMap*   specPrefilter;
    Texture2D* brdfIntMap;

    // Tone-mapped parameters.
    glm::vec3 exposure;
    GLfloat   gamma;

    Shader*  cubeShader;

    Mesh*    cube;
  };
}
