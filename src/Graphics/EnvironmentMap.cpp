#include "Graphics/EnvironmentMap.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/AssetManager.h"
#include "Graphics/Buffers.h"

namespace Strontium
{
  std::string
  EnvironmentMap::mapEnumToString(const MapType &type)
  {
    switch(type)
    {
      case MapType::Skybox: return "Skybox";
      case MapType::Irradiance: return "Diffuse Irradiance";
      case MapType::Prefilter: return "Specular Irradiance";
      case MapType::DynamicSky: return "Dynamic Sky";
      default: return "Unknown";
    }
  }

  EnvironmentMap::EnvironmentMap()
    : erMap(nullptr)
    , skybox(nullptr)
    , irradiance(nullptr)
    , specPrefilter(nullptr)
    , brdfIntMap(nullptr)
    , paramBuffer(sizeof(glm::vec4), BufferType::Dynamic)
    , preethamParams(2 * sizeof(glm::vec4), BufferType::Dynamic)
    , iblParams(sizeof(glm::vec4), BufferType::Dynamic)
    , equiToCubeCompute("./assets/shaders/compute/equiConversion.cs")
    , diffIrradCompute("./assets/shaders/compute/diffuseConv.cs")
    , specIrradCompute("./assets/shaders/compute/specPrefilter.cs")
    , brdfCompute("./assets/shaders/compute/integrateBRDF.cs")
    , preethamLUTCompute("./assets/shaders/compute/preethamLUT.cs")
    , cubeShader("./assets/shaders/forward/pbrSkybox.vs", "./assets/shaders/forward/pbrSkybox.fs")
    , preethamShader("./assets/shaders/forward/pbrSkybox.vs", "./assets/shaders/forward/preethamSkybox.fs")
    , currentEnvironment(MapType::Skybox)
    , currentDynamicSky(DynamicSkyType::Preetham)
    , intensity(1.0f)
    , roughness(0.0f)
    , filepath("")
  {
    // Load in the cube mesh and pre-integrate the BRDF LUT.
    this->cube.loadModel("./assets/.internal/cube.fbx");
    for (auto& submesh : this->cube.getSubmeshes())
    {
      if (!submesh.hasVAO())
        submesh.generateVAO();
    }
    this->computeBRDFLUT();

    // The Preetham dynamic sky model LUT.
    Texture2DParams params = Texture2DParams();
    params.internal = TextureInternalFormats::RGBA16f;
    params.format = TextureFormats::RGBA;
    params.dataType = TextureDataType::Floats;
    this->dynamicSkyLUT = createUnique<Texture2D>(1024, 512, 4, params);
    this->dynamicSkyLUT->initNullTexture();

    // Prepare parameters for dynamic sky models.
    this->dynamicSkyParams.emplace(DynamicSkyType::Preetham, new PreethamSkyParams());
    this->updateDynamicSky();
  }

  EnvironmentMap::~EnvironmentMap()
  {
    this->unloadEnvironment();

    // Clean up the dynamic sky data. TODO: unique pointer?
    for (auto& pair : this->dynamicSkyParams)
    {
      switch (pair.first)
      {
        case DynamicSkyType::Preetham:
        {
          delete static_cast<PreethamSkyParams*>(pair.second);
          break;
        }
      }
    }
  }

  // Unload all the textures associated from this environment.
  void
  EnvironmentMap::unloadEnvironment()
  {
    if (this->erMap != nullptr)
    {
      this->erMap = Unique<Texture2D>(nullptr);
    }
    if (this->skybox != nullptr)
    {
      this->skybox = Unique<CubeMap>(nullptr);
    }
    if (this->irradiance != nullptr)
    {
      this->irradiance = Unique<CubeMap>(nullptr);
    }
    if (this->specPrefilter != nullptr)
    {
      this->specPrefilter = Unique<CubeMap>(nullptr);
    }
  }

  void
  EnvironmentMap::unloadComputedMaps()
  {
    if (this->skybox != nullptr)
    {
      this->skybox = Unique<CubeMap>(nullptr);
    }
    if (this->irradiance != nullptr)
    {
      this->irradiance = Unique<CubeMap>(nullptr);
    }
    if (this->specPrefilter != nullptr)
    {
      this->specPrefilter = Unique<CubeMap>(nullptr);
    }
  }

  // Bind/unbind a specific cubemap.
  void
  EnvironmentMap::bind(const MapType &type)
  {
    switch (type)
    {
      case MapType::Skybox:
        if (this->skybox != nullptr)
          this->skybox->bind();
        break;
      case MapType::Irradiance:
        if (this->irradiance != nullptr)
          this->irradiance->bind();
        break;
      case MapType::Prefilter:
        if (this->specPrefilter != nullptr)
          this->specPrefilter->bind();
        break;
    }
  }

  void
  EnvironmentMap::unbind()
  {

  }

  void
  EnvironmentMap::bind(const MapType &type, GLuint bindPoint)
  {
    switch (type)
    {
      case MapType::Skybox:
        if (this->skybox != nullptr)
          this->skybox->bind(bindPoint);
        break;
      case MapType::Irradiance:
        if (this->irradiance != nullptr)
          this->irradiance->bind(bindPoint);
        break;
      case MapType::Prefilter:
        if (this->specPrefilter != nullptr)
          this->specPrefilter->bind(bindPoint);
        break;
      case MapType::DynamicSky:
        if (this->dynamicSkyLUT != nullptr)
          this->dynamicSkyLUT->bind(bindPoint);
        break;
    }
  }

  void
  EnvironmentMap::bindBRDFLUT(GLuint bindPoint)
  {
    if (this->brdfIntMap != nullptr)
      this->brdfIntMap->bind(bindPoint);
  }

  // Draw the skybox.
  void
  EnvironmentMap::configure()
  {
    this->paramBuffer.bindToPoint(1);
    this->paramBuffer.setData(0, sizeof(GLfloat), &this->roughness);
    this->bind(this->currentEnvironment, 0);
  }

  // Update the dynamic sky.
  void
  EnvironmentMap::updateDynamicSky()
  {
    switch (this->currentDynamicSky)
    {
      case DynamicSkyType::Preetham:
      {
        auto& skyParams = this->getSkyModelParams(DynamicSkyType::Preetham);
        auto& preethamSkyParams = *(static_cast<PreethamSkyParams*>(&skyParams));

        glm::vec3 sunDir = glm::vec3(sin(skyParams.inclination) * cos(skyParams.azimuth),
                                     cos(skyParams.inclination),
                                     sin(skyParams.inclination) * sin(skyParams.azimuth));
        auto preethamModel = glm::vec4(preethamSkyParams.turbidity, skyParams.sunSize,
                                       skyParams.sunIntensity, 0.0f);

        this->preethamParams.bindToPoint(1);
        this->preethamParams.setData(0, sizeof(glm::vec3), &sunDir.x);
        this->preethamParams.setData(sizeof(glm::vec3), sizeof(GLfloat), &preethamSkyParams.turbidity);
        this->preethamParams.setData(sizeof(glm::vec4), sizeof(GLfloat), &skyParams.sunIntensity);
        this->preethamParams.setData(sizeof(glm::vec4) + sizeof(GLfloat), sizeof(GLfloat), &skyParams.sunSize);

        this->dynamicSkyLUT->bindAsImage(0, 0, ImageAccessPolicy::Write);

        this->preethamLUTCompute.launchCompute(glm::ivec3(1024 / 32, 512 / 32, 1));
        ComputeShader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

        this->paramBuffer.setData(sizeof(GLfloat), sizeof(glm::vec3), &sunDir.x);
        break;
      }
    }
  }

  // Getters.
  GLuint
  EnvironmentMap::getTexID(const MapType &type)
  {
    switch (type)
    {
      case MapType::Skybox:
        if (this->skybox != nullptr)
          return this->skybox->getID();
      case MapType::Irradiance:
        if (this->irradiance != nullptr)
          return this->irradiance->getID();
      case MapType::Prefilter:
        if (this->specPrefilter != nullptr)
          return this->specPrefilter->getID();
      case MapType::DynamicSky:
        if (this->dynamicSkyLUT != nullptr)
          return this->dynamicSkyLUT->getID();
    }

    return 0;
  }

  // Load a 2D equirectangular map.
  void
  EnvironmentMap::loadEquirectangularMap(const std::string &filepath,
                                         const Texture2DParams &params)
  {
    this->erMap = Unique<Texture2D>(Texture2D::loadTexture2D(filepath, params, false));
    this->filepath = filepath;
  }

  // Convert the loaded equirectangular map to a cubemap.
  void
  EnvironmentMap::equiToCubeMap(const bool &isHDR, const GLuint &width,
                                const GLuint &height)
  {
    Logger* logs = Logger::getInstance();

    auto start = std::chrono::steady_clock::now();

    // The resulting cubemap from the conversion process.
    TextureCubeMapParams params = TextureCubeMapParams();
    params.minFilter = TextureMinFilterParams::LinearMipMapLinear;
    if (isHDR)
    {
      params.internal = TextureInternalFormats::RGBA16f;
      params.dataType = TextureDataType::Floats;
    }
    this->skybox = createUnique<CubeMap>(width, height, 4, params);
    this->skybox->initNullTexture();

    // Bind the equirectangular map for reading by the compute shader.
    this->erMap->bindAsImage(0, 0, ImageAccessPolicy::Read);

    // Bind the irradiance map for writing to by the compute shader.
    this->skybox->bindAsImage(1, 0, true, 0, ImageAccessPolicy::ReadWrite);

    // Launch the compute shader.
    this->equiToCubeCompute.launchCompute(glm::ivec3(width / 32, height / 32, 6));

    this->skybox->generateMips();

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    logs->logMessage(LogMessage("Converted the equirectangular map to a cubemap"
                                " (elapsed time: " + std::to_string(elapsed.count())
                                + " s).", true, true));
  }

  // Generate the diffuse irradiance map.
  void
  EnvironmentMap::precomputeIrradiance(const GLuint &width, const GLuint &height,
                                       bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    auto start = std::chrono::steady_clock::now();

    // The resulting cubemap from the convolution process.
    TextureCubeMapParams params = TextureCubeMapParams();
    if (isHDR)
    {
      params.internal = TextureInternalFormats::RGBA16f;
      params.dataType = TextureDataType::Floats;
    }
    this->irradiance = createUnique<CubeMap>(width, height, 4, params);
    this->irradiance->initNullTexture();

    // Bind the skybox for reading by the compute shader.
    this->skybox->bindAsImage(0, 0, true, 0, ImageAccessPolicy::Read);

    // Bind the irradiance map for writing to by the compute shader.
    this->irradiance->bindAsImage(1, 0, true, 0, ImageAccessPolicy::Write);

    // Launch the compute shader.
    this->diffIrradCompute.launchCompute(glm::ivec3(width / 32, height / 32, 6));

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    logs->logMessage(LogMessage("Convoluted environment map (elapsed time: "
                                + std::to_string(elapsed.count()) + " s).", true,
                                true));
  }

  // Generate the specular map components. Computes the pre-filtered environment
  // map first, than integrates the BRDF and stores the result in an LUT.
  void
  EnvironmentMap::precomputeSpecular(const GLuint &width, const GLuint &height,
                                     bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    if (this->specPrefilter == nullptr)
    {
      //--------------------------------------------------------------------------
      // The pre-filtered environment map component.
      //--------------------------------------------------------------------------
      auto start = std::chrono::steady_clock::now();

      // The resulting cubemap from the environment pre-filter.
      TextureCubeMapParams params = TextureCubeMapParams();
      params.minFilter = TextureMinFilterParams::LinearMipMapLinear;
      if (isHDR)
      {
        params.internal = TextureInternalFormats::RGBA16f;
        params.dataType = TextureDataType::Floats;
      }
      this->specPrefilter = createUnique<CubeMap>(width, height, 4, params);
      this->specPrefilter->initNullTexture();
      this->specPrefilter->generateMips();

      this->iblParams.bindToPoint(2);
      glm::vec4 paramsToUpload = glm::vec4(0.0f);

      // Bind the enviroment map which is to be prefiltered.
      this->skybox->bind(0);

      // Perform the pre-filter for each roughness level.
      for (GLuint i = 0; i < 5; i++)
      {
        // Compute the current mip levels.
        GLuint mipWidth  = (GLuint) ((GLfloat) width * std::pow(0.5f, i));
        GLuint mipHeight = (GLuint) ((GLfloat) height * std::pow(0.5f, i));

        // Bind the irradiance map for writing to by the compute shader.
        this->specPrefilter->bindAsImage(1, i, true, 0, ImageAccessPolicy::Write);

        // Roughness.
        paramsToUpload.x = ((GLfloat) i) / ((GLfloat) (5 - 1));
        this->iblParams.setData(0, sizeof(glm::vec4), &paramsToUpload.x);

        // Launch the compute.
        this->specIrradCompute.launchCompute(glm::ivec3(mipWidth / 32, mipHeight / 32, 6));
      }

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Pre-filtered environment map (elapsed time: "
                                  + std::to_string(elapsed.count()) + " s).", true,
                                  true));
    }
  }

  void
  EnvironmentMap::computeBRDFLUT()
  {
    Logger* logs = Logger::getInstance();

    if (this->brdfIntMap == nullptr)
    {
      //--------------------------------------------------------------------------
      // The BRDF integration map component.
      //--------------------------------------------------------------------------
      auto start = std::chrono::steady_clock::now();

      Texture2DParams params = Texture2DParams();
      params.sWrap = TextureWrapParams::ClampEdges;
      params.tWrap = TextureWrapParams::ClampEdges;
      params.internal = TextureInternalFormats::RG16f;
      params.dataType = TextureDataType::Floats;
      this->brdfIntMap = createUnique<Texture2D>(512, 512, 2, params);
      this->brdfIntMap->initNullTexture();

      // Bind the integration map for writing to by the compute shader.
      this->brdfIntMap->bindAsImage(3, 0, ImageAccessPolicy::Write);

      // Launch the compute shader.
      this->brdfCompute.launchCompute(glm::ivec3(512 / 32, 512 / 32, 1));

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Generated BRDF lookup texture (elapsed time: "
                                  + std::to_string(elapsed.count()) + " s).", true,
                                  true));
    }
  }

  Shader*
  EnvironmentMap::getCubeProg()
  {
    switch (this->currentEnvironment)
    {
      case MapType::DynamicSky: return &this->preethamShader; break;
      default:                  return &this->cubeShader; break;
    }
  }

  void
  EnvironmentMap::setSkyModelParams(DynamicSkyCommonParams* params)
  {
    switch (params->type)
    {
      case DynamicSkyType::Preetham:
      {
        auto preethamParams = *(static_cast<PreethamSkyParams*>(params));
        bool shouldUpdate = *(static_cast<PreethamSkyParams*>(this->dynamicSkyParams[params->type])) != preethamParams;

        if (shouldUpdate)
        {
          // TODO: figure out how to do this with C++.
          memcpy(this->dynamicSkyParams[params->type], &preethamParams, sizeof(PreethamSkyParams));
          this->updateDynamicSky();
        }
        break;
      }
    }
  }
}
