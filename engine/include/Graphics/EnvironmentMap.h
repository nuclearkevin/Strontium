// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Model.h"
#include "Graphics/Shaders.h"
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
    /*
    glm::vec3 rayleighScatteringBase;
    float rayleighAbsorptionBase;
    float mieScatteringBase;
    float mieAbsorptionBase;
    glm::vec3 ozoneAbsorptionBase;

    float planetRadius;
    float atmosphereRadius;

    glm::vec3 viewPos;
    */
    // Planetary radii are in Mm. Height falloffs are in km.
    glm::vec4 rayleighScat; //  Rayleigh scattering base (x, y, z) and height falloff (w).
    glm::vec4 rayleighAbs; //  Rayleigh absorption base (x, y, z) and height falloff (w).
    glm::vec4 mieScat; //  Mie scattering base (x, y, z) and height falloff (w).
    glm::vec4 mieAbs; //  Mie absorption base (x, y, z) and height falloff (w).
    glm::vec4 ozoneAbs; //  Ozone absorption base (x, y, z) and height falloff (w).
    glm::vec4 planetAlbedoRadius; // Planet albedo (x, y, z) and radius.
    glm::vec4 sunDirAtmRadius; // Sun direction (x, y, z) and atmosphere radius (w).
    glm::vec4 viewPos; // View position (x, y, z). w is unused.

    HillaireSkyParams()
      : DynamicSkyCommonParams(DynamicSkyType::Hillaire)
      , rayleighScat(5.802f, 13.558f, 33.1f, 8.0f)
      , rayleighAbs(0.0f, 0.0f, 0.0f, 8.0f)
      , mieScat(3.996f, 3.996f, 3.996f, 1.2f)
      , mieAbs(4.4f, 4.4f, 4.4f, 1.2f)
      , ozoneAbs(0.650f, 1.881f, 0.085f, 0.002f)
      , planetAlbedoRadius(0.0f, 0.0f, 0.0f, 6.360f)
      , sunDirAtmRadius(sunPos, 6.460f)
      , viewPos(0.0f, 6.360f + 0.0002f, 0.0f, 0.0f)
    { }

    bool operator==(const HillaireSkyParams& other)
    {
      return this->rayleighScat == other.rayleighScat &&
             this->rayleighAbs == other.rayleighAbs &&
             this->mieScat == other.mieScat &&
             this->mieAbs == other.mieAbs &&
             this->ozoneAbs == other.ozoneAbs &&
             this->planetAlbedoRadius == other.planetAlbedoRadius &&
             this->sunDirAtmRadius == other.sunDirAtmRadius &&
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
    // Generate the diffuse irradiance cubemap.
    void precomputeIrradiance(const uint& width = 512, const uint& height = 512, bool isHDR = true);
    // Generate the specular prefilter cubemap.
    void precomputeSpecular(const uint& width = 512, const uint& height = 512, bool isHDR = true);

    // Bind a specific map.
    void bind(const MapType &type);
    void bind(const MapType &type, uint bindPoint);
    void bindBRDFLUT(uint bindPoint);
    void bindAerialPerspectiveLUT(uint bindPoint);

    // Draw the skybox.
    void configure();

    // Update the dynamic sky.
    void updateDynamicSky();
    void updateAerialPerspective(UniformBuffer &cameraBuffer);

    // Set the environment map to constantly update the diffuse irradiance
    // and specular prefilter cubemaps. Forces the resolution of the diffuse 
    // cubemap to 32x32 and the specular cubemap to 64x64.
    void setDynamicSkyIBL();
    void setStaticIBL();

    // Getters.
    uint getTexID(const MapType &type);
    uint getBRDFLUTID() { return this->brdfIntLUT.getID(); }
    uint getTransmittanceLUTID() { return this->transmittanceLUT.getID(); }
    uint getMultiScatteringLUTID() { return this->multiScatLUT.getID(); }

    float& getIntensity() { return this->intensity; }
    float& getRoughness() { return this->roughness; }

    MapType getDrawingType() { return this->currentEnvironment; }
    void setDrawingType(const MapType &type) { this->currentEnvironment = type; }

    DynamicSkyType getDynamicSkyType() { return this->currentDynamicSky; }
    void setDynamicSkyType(const DynamicSkyType &type);

    template <typename T>
    T& getSkyParams(const DynamicSkyType &type)
    {
      static_assert(std::is_base_of<DynamicSkyCommonParams, T>::value, "Class must derive from DynamicSkyCommonParams.");

      DynamicSkyCommonParams* retValue = this->dynamicSkyParams.at(type);
      assert(("DynamicSkyCommonParams does not exist.", retValue));

      return (*static_cast<T*>(retValue));
    }

    template <typename T>
    void setSkyModelParams(T &params)
    {
      static_assert(std::is_base_of<DynamicSkyCommonParams, T>::value, "Class must derive from DynamicSkyCommonParams.");

      auto storedParams = *(static_cast<T*>(this->dynamicSkyParams[params.type]));

      if (storedParams != params)
      {
        memcpy(this->dynamicSkyParams[params.type], &params, sizeof(T));
        this->updateDynamicSky();
      }
    }

    std::string& getFilepath() { return this->filepath; }
  protected:
    Unique<Texture2D> erMap;

    CubeMap skybox;
    CubeMap irradiance;
    CubeMap specPrefilter;
    Texture2D brdfIntLUT;

    Texture2D transmittanceLUT;
    Texture2D multiScatLUT;
    Texture2D skyViewLUT;
    Texture3D aerialPerspectiveLUT;

    UniformBuffer skyboxParamBuffer;
    ShaderStorageBuffer preethamParams;
    UniformBuffer hillaireParams;
    ShaderStorageBuffer iblParams;

    std::unordered_map<DynamicSkyType, DynamicSkyCommonParams*> dynamicSkyParams;

    std::string filepath;
    MapType currentEnvironment;
    DynamicSkyType currentDynamicSky;
    bool staticIBL;

    // Parameters for drawing the skybox.
    float intensity;
    float roughness;
    glm::ivec4 skyboxParameters;
  };
}
