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
    Preetham = 3
  };

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

    GLfloat& getInclination() { return this->inclination; }
    GLfloat& getAzimuth() { return this->azimuth; }
    GLfloat& getTurbidity() { return this->turbidity; }
    GLfloat& getSunSize() { return this->sunSize; }
    GLfloat& getSunIntensity() { return this->sunIntensity; }

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
  protected:
    Unique<Texture2D> erMap;
    Unique<CubeMap>   skybox;
    Unique<CubeMap>   irradiance;
    Unique<CubeMap>   specPrefilter;
    Unique<Texture2D> brdfIntMap;
    Unique<Texture2D> preethamLUT;

    ComputeShader equiToCubeCompute;
    ComputeShader diffIrradCompute;
    ComputeShader specIrradCompute;
    ComputeShader brdfCompute;
    ComputeShader preethamLUTCompute;

    UniformBuffer paramBuffer;
    ShaderStorageBuffer preethamParams;

    Shader cubeShader;
    Shader preethamShader;

    GLfloat turbidity;
    GLfloat azimuth;
    GLfloat inclination;
    GLfloat sunSize;
    GLfloat sunIntensity;

    std::string filepath;
    MapType currentEnvironment;

    // Parameters for drawing the skybox.
    GLfloat intensity;
    GLfloat roughness;


    Model    cube;
  };
}
