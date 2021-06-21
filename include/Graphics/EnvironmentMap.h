// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Model.h"
#include "Graphics/Shaders.h"
#include "Graphics/Camera.h"
#include "Graphics/Textures.h"

namespace SciRenderer
{
  enum class MapType { Equirectangular, Skybox, Irradiance, Prefilter, Integration };

  class EnvironmentMap
  {
  public:
    EnvironmentMap(const std::string &cubeMeshPath);
    ~EnvironmentMap();

    // Load a 2D equirectangular map. Assumes that the map is HDR by default.
    void loadEquirectangularMap(const std::string &filepath,
                                const Texture2DParams &params = Texture2DParams());

    // Convert an equirectangular map to a cubemap.
    void equiToCubeMap(const bool &isHDR = true, const GLuint &width = 512,
                       const GLuint &height = 512);

    // Unload all the textures associated with this environment.
    void unloadEnvironment();
    // Unload the non-equirectangular map textures.
    void unloadComputedMaps();

    // Bind/unbind a specific cubemap.
    void bind(const MapType &type);
    void unbind();

    // Binds one of the environment map PBR textures to a point.
    void bind(const MapType &type, GLuint bindPoint);

    // Draw the skybox.
    void configure(Shared<Camera> camera);

    // Generate the diffuse irradiance map.
    void precomputeIrradiance(const GLuint &width = 512, const GLuint &height = 512, bool isHDR = true);

    // Generate the specular map components (pre-filter and BRDF integration map).
    void precomputeSpecular(const GLuint &width = 512, const GLuint &height = 512, bool isHDR = true);

    // Getters.
    GLuint getTexID(const MapType &type);

    GLfloat& getGamma() { return this->gamma; }
    GLfloat& getRoughness() { return this->roughness; }
    Model* getCubeMesh() { return this->cube; }
    Shader* getCubeProg() { return this->cubeShader; }
    std::string& getFilepath() { return this->filepath; }
    bool hasEqrMap() { return this->erMap != nullptr; }
    bool hasSkybox() { return this->skybox != nullptr; }
    bool hasIrradiance() { return this->irradiance != nullptr; }
    bool hasPrefilter() { return this->specPrefilter != nullptr; }
    bool hasIntegration() { return this->brdfIntMap != nullptr; }
    bool drawingSkybox() { return this->currentEnvironment == MapType::Skybox; }
    bool drawingIrrad() { return this->currentEnvironment == MapType::Irradiance; }
    bool drawingFilter() { return this->currentEnvironment == MapType::Prefilter; }

    void setDrawingType(MapType type) { this->currentEnvironment = type; }
  protected:
    Unique<Texture2D> erMap;
    Unique<CubeMap>   skybox;
    Unique<CubeMap>   irradiance;
    Unique<CubeMap>   specPrefilter;
    Unique<Texture2D> brdfIntMap;

    std::string filepath;

    MapType   currentEnvironment;

    // Tone-mapped parameters.
    GLfloat   gamma;
    GLfloat   roughness;

    Shader*  cubeShader;

    Model*    cube;
  };
}
