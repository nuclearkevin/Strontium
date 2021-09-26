// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Model.h"
#include "Graphics/Shaders.h"
#include "Graphics/Compute.h"
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
    Preetham = 0,
    Hillaire = 1
  };

  // Parameters for dynamic sky models.
  struct DynamicSkyCommonParams
  {
    glm::vec3 sunPos;
    float sunSize;
    float sunIntensity;
    float skyIntensity;

    const DynamicSkyType type;

    DynamicSkyCommonParams(const DynamicSkyType &type)
      : sunPos(0.0f, 1.0f, 0.0f)
      , sunSize(1.0f)
      , sunIntensity(1.0f)
      , skyIntensity(1.0f)
      , type(type)
    { }

    bool operator ==(const DynamicSkyCommonParams& other)
    {
      return this->sunPos == other.sunPos &&
             this->sunSize == other.sunSize &&
             this->sunIntensity == other.sunIntensity &&
             this->skyIntensity == other.skyIntensity &&
             this->type == other.type;
    }

    bool operator !=(const DynamicSkyCommonParams& other) { return !((*this) == other); }
  };

  struct PreethamSkyParams : public DynamicSkyCommonParams
  {
    float turbidity;

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

  struct HillaireSkyParams : public DynamicSkyCommonParams
  {
    glm::vec3 rayleighScatteringBase;
    float rayleighAbsorptionBase;
    float mieScatteringBase;
    float mieAbsorptionBase;
    glm::vec3 ozoneAbsorptionBase;

    float planetRadius;
    float atmosphereRadius;

    glm::vec3 viewPos;

    HillaireSkyParams()
      : DynamicSkyCommonParams(DynamicSkyType::Hillaire)
      , rayleighScatteringBase(5.802f, 13.558f, 33.1f)
      , rayleighAbsorptionBase(0.0f)
      , mieScatteringBase(3.996f)
      , mieAbsorptionBase(4.4f)
      , ozoneAbsorptionBase(0.650f, 1.881f, 0.085f)
      , planetRadius(6.360f)
      , atmosphereRadius(6.460f)
      , viewPos(0.0f, 6.360f + 0.0002f, 0.0f)
    { }

    bool operator==(const HillaireSkyParams& other)
    {
      return this->rayleighScatteringBase == other.rayleighScatteringBase &&
             this->rayleighAbsorptionBase == other.rayleighAbsorptionBase &&
             this->mieScatteringBase == other.mieScatteringBase &&
             this->mieAbsorptionBase == other.mieAbsorptionBase &&
             this->ozoneAbsorptionBase == other.ozoneAbsorptionBase &&
             this->planetRadius == other.planetRadius &&
             this->atmosphereRadius == other.atmosphereRadius &&
             this->viewPos == other.viewPos &&
             DynamicSkyCommonParams::operator==(other);
    }

    bool operator !=(const HillaireSkyParams& other) { return !((*this) == other); }
  };

  // The environment map class. Responsible for ambient lighting and IBL.
  class EnvironmentMap
  {
  public:
    static std::string mapEnumToString(const MapType &type);
    static std::string skyEnumToString(const DynamicSkyType &type);

    EnvironmentMap();
    ~EnvironmentMap();

    // Load a 2D equirectangular map. Assumes that the map is HDR by default.
    void loadEquirectangularMap(const std::string &filepath,
                                const Texture2DParams &params = Texture2DParams());

    // Convert an equirectangular map to a cubemap.
    void equiToCubeMap(const bool &isHDR = true, const uint &width = 512,
                       const uint &height = 512);

    // Unload all the textures associated with this environment.
    void unloadEnvironment();
    // Unload the non-equirectangular map textures.
    void unloadComputedMaps();

    // Bind/unbind a specific cubemap.
    void bind(const MapType &type);
    void unbind();

    // Binds one of the environment map PBR textures to a point.
    void bind(const MapType &type, uint bindPoint);
    void bindBRDFLUT(uint bindPoint);

    // Draw the skybox.
    void configure();

    // Update the dynamic sky.
    void updateDynamicSky();
    void updateHillaireLUTs();

    // Generate the diffuse irradiance map.
    void precomputeIrradiance(const uint &width = 512, const uint &height = 512, bool isHDR = true);

    // Generate the specular map components (pre-filter and BRDF integration map).
    void precomputeSpecular(const uint &width = 512, const uint &height = 512, bool isHDR = true);

    // Compute the BRDF integration LUT separately.
    void computeBRDFLUT();

    // Getters.
    uint getTexID(const MapType &type);
    uint getBRDFLUTID() { return this->brdfIntLUT.getID(); }
    uint getTransmittanceLUTID() { return this->transmittanceLUT.getID(); }
    uint getMultiScatteringLUTID() { return this->multiScatLUT.getID(); }
    uint getSkyViewLUTID() { return this->skyViewLUT.getID(); }

    float& getIntensity() { return this->intensity; }
    float& getRoughness() { return this->roughness; }

    DynamicSkyCommonParams& getSkyModelParams(const DynamicSkyType &type) { return (*this->dynamicSkyParams.at(type)); }

    MapType getDrawingType() { return this->currentEnvironment; }
    DynamicSkyType getDynamicSkyType() { return this->currentDynamicSky; }
    Model* getCubeMesh() { return &this->cube; }
    Shader* getCubeProg();
    std::string& getFilepath() { return this->filepath; }

    void setDrawingType(const MapType &type) { this->currentEnvironment = type; }
    void setSkyboxType(const DynamicSkyType &type);
    void setSkyModelParams(DynamicSkyCommonParams* params);
  protected:
    Unique<Texture2D> erMap;
    CubeMap skybox;
    CubeMap irradiance;
    CubeMap specPrefilter;
    Texture2D brdfIntLUT;

    Texture2D dynamicSkyLUT;
    Texture2D transmittanceLUT;
    Texture2D multiScatLUT;
    Texture2D skyViewLUT;

    ComputeShader equiToCubeCompute;
    ComputeShader diffIrradCompute;
    ComputeShader specIrradCompute;
    ComputeShader brdfCompute;

    ComputeShader preethamLUTCompute;
    ComputeShader transmittanceCompute;
    ComputeShader multiScatCompute;
    ComputeShader skyViewCompute;

    UniformBuffer skyboxParamBuffer;
    ShaderStorageBuffer preethamParams;
    ShaderStorageBuffer hillaireParams;
    ShaderStorageBuffer iblParams;

    Shader dynamicSkyShader;

    std::unordered_map<DynamicSkyType, DynamicSkyCommonParams*> dynamicSkyParams;

    std::string filepath;
    MapType currentEnvironment;
    DynamicSkyType currentDynamicSky;

    // Parameters for drawing the skybox.
    float intensity;
    float roughness;

    glm::ivec4 skyboxParameters;

    Model    cube;
  };
}
