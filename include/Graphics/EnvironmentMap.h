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
  enum class MapType { Equirectangular, Skybox, Irradiance, Prefilter, Integration };

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

    // Load a 2D equirectangular map. Assumes that the map is HDR by default.
    void loadEquirectangularMap(const std::string &filepath,
                                const Texture2DParams &params = Texture2DParams(),
                                const bool &isHDR = true);

    // Convert an equirectangular map to a cubemap.
    void equiToCubeMap(const bool &isHDR = true, const GLuint &width = 512,
                       const GLuint &height = 512);

    // Unload all the textures associated with this environment.
    void unloadEnvironment();

    // Bind/unbind a specific cubemap.
    void bind(const MapType &type);
    void unbind();

    // Binds one of the environment map PBR textures to a point.
    void bind(const MapType &type, GLuint bindPoint);

    // Draw the skybox.
    void draw(Camera* camera);

    // Generate the diffuse irradiance map.
    void precomputeIrradiance(const GLuint &width = 512, const GLuint &height = 512, bool isHDR = true);

    // Generate the specular map components (pre-filter and BRDF integration map).
    void precomputeSpecular(const GLuint &width = 512, const GLuint &height = 512, bool isHDR = true);

    // Getters.
    GLuint getTexID(const MapType &type);

    inline GLfloat& getGamma() { return this->gamma; }
    inline GLfloat& getExposure() { return this->exposure; }
    inline GLfloat& getRoughness() { return this->roughness; }
    inline bool hasEqrMap() { return this->erMap != nullptr; }
    inline bool hasSkybox() { return this->skybox != nullptr; }
    inline bool hasIrradiance() { return this->irradiance != nullptr; }
    inline bool hasPrefilter() { return this->specPrefilter != nullptr; }
    inline bool hasIntegration() { return this->brdfIntMap != nullptr; }
    inline bool drawingSkybox() { return this->currentEnvironment == MapType::Skybox; }
    inline bool drawingIrrad() { return this->currentEnvironment == MapType::Irradiance; }
    inline bool drawingFilter() { return this->currentEnvironment == MapType::Prefilter; }

    inline void setDrawingType(MapType type) { this->currentEnvironment = type; }
  protected:
    Texture2D* erMap;
    CubeMap*   skybox;
    CubeMap*   irradiance;
    CubeMap*   specPrefilter;
    Texture2D* brdfIntMap;

    MapType   currentEnvironment;

    // Tone-mapped parameters.
    GLfloat   exposure;
    GLfloat   gamma;
    GLfloat   roughness;

    Shader*  cubeShader;

    Mesh*    cube;

    static FBOTex2DParam faces[6];
  };
}
