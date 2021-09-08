// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Model.h"
#include "Graphics/Shaders.h"
#include "Graphics/Compute.h"
#include "Graphics/Camera.h"
#include "Graphics/Textures.h"

namespace Strontium
{
  enum class MapType
  {
    Skybox = 0,
    Irradiance = 1,
    Prefilter = 2,
    DynamicSky = 3
  };

  enum class DynamicSkyType
  {
    Preetham = 0
  };

  // Parameters for dynamic sky models.
  struct DynamicSkyCommonParams
  {
    GLfloat azimuth;
    GLfloat inclination;
    GLfloat sunSize;
    GLfloat sunIntensity;

    const DynamicSkyType type;

    DynamicSkyCommonParams(const DynamicSkyType &type)
      : azimuth(0.0f)
      , inclination(0.0f)
      , sunSize(1.0f)
      , sunIntensity(1.0f)
      , type(type)
    { }

    bool operator ==(const DynamicSkyCommonParams& other)
    {
      return this->azimuth == other.azimuth &&
             this->inclination == other.inclination &&
             this->sunSize == other.sunSize &&
             this->sunIntensity == other.sunIntensity &&
             this->type == other.type;
    }

    bool operator !=(const DynamicSkyCommonParams& other) { return !((*this) == other); }
  };

  struct PreethamSkyParams : public DynamicSkyCommonParams
  {
    GLfloat turbidity;

    PreethamSkyParams()
      : DynamicSkyCommonParams(DynamicSkyType::Preetham)
      , turbidity(2.0f)
    { }

    bool operator==(const PreethamSkyParams& other)
    {
      return this->turbidity == other.turbidity &&
             DynamicSkyCommonParams::operator==(other);
    }

    bool operator !=(const PreethamSkyParams& other) { return !((*this) == other); }
  };

  // The environment map class. Responsible for ambient lighting and IBL.
  class EnvironmentMap
  {
  public:
    static std::string mapEnumToString(const MapType &type);

    EnvironmentMap();
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
    void bindBRDFLUT(GLuint bindPoint);

    // Draw the skybox.
    void configure();

    // Update the dynamic sky.
    void updateDynamicSky();

    // Generate the diffuse irradiance map.
    void precomputeIrradiance(const GLuint &width = 512, const GLuint &height = 512, bool isHDR = true);

    // Generate the specular map components (pre-filter and BRDF integration map).
    void precomputeSpecular(const GLuint &width = 512, const GLuint &height = 512, bool isHDR = true);

    // Compute the BRDF integration LUT separately.
    void computeBRDFLUT();

    // Getters.
    GLuint getTexID(const MapType &type);

    GLfloat& getIntensity() { return this->intensity; }
    GLfloat& getRoughness() { return this->roughness; }

    DynamicSkyCommonParams& getSkyModelParams(const DynamicSkyType &type) { return (*this->dynamicSkyParams.at(type)); }

    MapType getDrawingType() { return this->currentEnvironment; }
    Model* getCubeMesh() { return &this->cube; }
    Shader* getCubeProg();
    std::string& getFilepath() { return this->filepath; }

    bool hasEqrMap() { return this->erMap != nullptr; }
    bool hasSkybox() { return this->skybox != nullptr; }
    bool hasIrradiance() { return this->irradiance != nullptr; }
    bool hasPrefilter() { return this->specPrefilter != nullptr; }
    bool hasIntegration() { return this->brdfIntMap != nullptr; }

    void setDrawingType(MapType type) { this->currentEnvironment = type; }
    void setSkyModelParams(DynamicSkyCommonParams* params);
  protected:
    Unique<Texture2D> erMap;
    Unique<CubeMap> skybox;
    Unique<CubeMap> irradiance;
    Unique<CubeMap> specPrefilter;
    Unique<Texture2D> brdfIntMap;
    Unique<Texture2D> dynamicSkyLUT;

    ComputeShader equiToCubeCompute;
    ComputeShader diffIrradCompute;
    ComputeShader specIrradCompute;
    ComputeShader brdfCompute;
    ComputeShader preethamLUTCompute;

    UniformBuffer paramBuffer;
    ShaderStorageBuffer preethamParams;
    ShaderStorageBuffer iblParams;

    Shader cubeShader;
    Shader preethamShader;

    std::unordered_map<DynamicSkyType, DynamicSkyCommonParams*> dynamicSkyParams;

    std::string filepath;
    MapType currentEnvironment;
    DynamicSkyType currentDynamicSky;

    // Parameters for drawing the skybox.
    GLfloat intensity;
    GLfloat roughness;

    Model    cube;
  };
}
