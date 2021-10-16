#include "Graphics/EnvironmentMap.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/AssetManager.h"
#include "Graphics/Buffers.h"
#include "Graphics/Renderer.h"

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

  std::string
  EnvironmentMap::skyEnumToString(const DynamicSkyType &type)
  {
    switch(type)
    {
      case DynamicSkyType::Preetham: return "Preetham";
      case DynamicSkyType::Hillaire: return "UE4";
      default: return "Unknown";
    }
  }

  EnvironmentMap::EnvironmentMap()
    : erMap(nullptr)
    , skyboxParamBuffer(3 * sizeof(glm::vec4) + sizeof(glm::ivec4), BufferType::Dynamic)
    , preethamParams(2 * sizeof(glm::vec4), BufferType::Dynamic)
    , hillaireParams(5 * sizeof(glm::vec4), BufferType::Dynamic)
    , iblParams(sizeof(glm::vec4) + sizeof(glm::ivec4), BufferType::Dynamic)
    , equiToCubeCompute("./assets/shaders/compute/ibl/equiConversion.srshader")
    , diffIrradCompute("./assets/shaders/compute/ibl/diffuseConv.srshader")
    , skyDiffCompute("./assets/shaders/compute/ibl/diffSkyIBL.srshader")
    , specIrradCompute("./assets/shaders/compute/ibl/specPrefilter.srshader")
    , skySpecCompute("./assets/shaders/compute/ibl/specSkyIBL.srshader")
    , brdfCompute("./assets/shaders/compute/ibl/integrateBRDF.srshader")
    , preethamLUTCompute("./assets/shaders/compute/sky/preethamLUT.srshader")
    , transmittanceCompute("./assets/shaders/compute/sky/transCompute.srshader")
    , multiScatCompute("./assets/shaders/compute/sky/multiscatCompute.srshader")
    , skyViewCompute("./assets/shaders/compute/sky/skyviewCompute.srshader")
    , dynamicSkyShader("./assets/shaders/forward/skybox.srshader")
    , currentEnvironment(MapType::Skybox)
    , currentDynamicSky(DynamicSkyType::Preetham)
    , intensity(1.0f)
    , roughness(0.0f)
    , skyboxParameters(0)
    , filepath("")
    , staticIBL(true)
  {
    // Load in the cube mesh and pre-integrate the BRDF LUT.
    this->cube.loadModel("./assets/.internal/cube.fbx");
    for (auto& submesh : this->cube.getSubmeshes())
    {
      if (!submesh.hasVAO())
        submesh.generateVAO();
    }
    
    // Compute the BRDF LUT.
    {
        Logger* logs = Logger::getInstance();
        auto start = std::chrono::steady_clock::now();

        Texture2DParams params = Texture2DParams();
        params.sWrap = TextureWrapParams::ClampEdges;
        params.tWrap = TextureWrapParams::ClampEdges;
        params.internal = TextureInternalFormats::RG16f;
        params.dataType = TextureDataType::Floats;

        this->brdfIntLUT.setSize(32, 32, 2);
        this->brdfIntLUT.setParams(params);
        this->brdfIntLUT.initNullTexture();

        // Bind the integration map for writing to by the compute shader.
        this->brdfIntLUT.bindAsImage(3, 0, ImageAccessPolicy::Write);

        // Launch the compute shader.
        this->brdfCompute.launchCompute(32 / 32, 32 / 32, 1);

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        logs->logMessage(LogMessage("Generated BRDF lookup texture (elapsed time: "
            + std::to_string(elapsed.count()) + " s).", true,
            true));
    }

    // The dynamic sky model LUT.
    Texture2DParams params = Texture2DParams();
    params.sWrap = TextureWrapParams::ClampEdges;
    params.tWrap = TextureWrapParams::ClampEdges;
    params.internal = TextureInternalFormats::RGBA16f;
    params.format = TextureFormats::RGBA;
    params.dataType = TextureDataType::Floats;

    // The transmittance LUT.
    this->transmittanceLUT.width = 256;
    this->transmittanceLUT.height = 64;
    this->transmittanceLUT.n = 4;
    this->transmittanceLUT.setParams(params);
    this->transmittanceLUT.initNullTexture();

    // The multi-scattering LUT.
    this->multiScatLUT.width = 32;
    this->multiScatLUT.height = 32;
    this->multiScatLUT.n = 4;
    this->multiScatLUT.setParams(params);
    this->multiScatLUT.initNullTexture();

    // The sky-view LUT.
    this->skyViewLUT.width = 256;
    this->skyViewLUT.height = 128;
    this->skyViewLUT.n = 4;
    this->skyViewLUT.setParams(params);
    this->skyViewLUT.initNullTexture();

    // Prepare parameters for dynamic sky models.
    this->dynamicSkyParams.emplace(DynamicSkyType::Preetham, new PreethamSkyParams());
    this->dynamicSkyParams.emplace(DynamicSkyType::Hillaire, new HillaireSkyParams());

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
        case DynamicSkyType::Hillaire:
        {
          delete static_cast<HillaireSkyParams*>(pair.second);
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
      this->erMap->clearTexture();
    this->skybox.clearTexture();
    this->irradiance.clearTexture();
    this->specPrefilter.clearTexture();
  }

  void
  EnvironmentMap::unloadComputedMaps()
  {
    this->skybox.clearTexture();
    this->irradiance.clearTexture();
    this->specPrefilter.clearTexture();
  }

  // Bind/unbind a specific cubemap.
  void
  EnvironmentMap::bind(const MapType &type)
  {
    switch (type)
    {
      case MapType::Skybox:
      {
        this->skybox.bind();
        break;
      }
      case MapType::Irradiance:
      {
        this->irradiance.bind();
        break;
      }
      case MapType::Prefilter:
      {
        this->specPrefilter.bind();
        break;
      }
    }
  }

  void
  EnvironmentMap::bind(const MapType &type, uint bindPoint)
  {
    switch (type)
    {
      case MapType::Skybox:
      {
        this->skybox.bind(bindPoint);
        break;
      }
      case MapType::Irradiance:
      {
        this->irradiance.bind(bindPoint);
        break;
      }
      case MapType::Prefilter:
      {
        this->specPrefilter.bind(bindPoint);
        break;
      }
    }
  }

  void
  EnvironmentMap::bindBRDFLUT(uint bindPoint)
  {
    this->brdfIntLUT.bind(bindPoint);
  }

  // Draw the skybox.
  void
  EnvironmentMap::configure()
  {
    glm::vec4 params0 = glm::vec4(0.0f);
    glm::vec4 params1 = glm::vec4(0.0f);
    glm::vec4 params2 = glm::vec4(0.0f);

    this->skyboxParamBuffer.bindToPoint(1);
    if (this->currentEnvironment == MapType::DynamicSky)
    {
      this->skyboxParameters.x = 1;
      switch (this->currentDynamicSky)
      {
        case DynamicSkyType::Preetham:
        {
          auto& preethamSkyParams = this->getSkyParams<PreethamSkyParams>(DynamicSkyType::Preetham);

          params0.y = preethamSkyParams.sunPos.x;
          params0.z = preethamSkyParams.sunPos.y;
          params0.w = preethamSkyParams.sunPos.z;

          params1.x = preethamSkyParams.sunIntensity;
          params1.y = preethamSkyParams.sunSize;

          params2.w = preethamSkyParams.skyIntensity;

          this->skyViewLUT.bind(1);
          break;
        }

        case DynamicSkyType::Hillaire:
        {
          auto& hillaireSkyParams = this->getSkyParams<HillaireSkyParams>(DynamicSkyType::Hillaire);

          this->skyboxParameters.x = 2;

          params0.y = hillaireSkyParams.sunPos.x;
          params0.z = hillaireSkyParams.sunPos.y;
          params0.w = hillaireSkyParams.sunPos.z;

          params1.x = hillaireSkyParams.sunIntensity;
          params1.y = hillaireSkyParams.sunSize;
          params1.z = hillaireSkyParams.planetRadius;
          params1.w = hillaireSkyParams.atmosphereRadius;

          params2.x = hillaireSkyParams.viewPos.x;
          params2.y = hillaireSkyParams.viewPos.y;
          params2.z = hillaireSkyParams.viewPos.z;
          params2.w = hillaireSkyParams.skyIntensity;

          this->skyViewLUT.bind(1);
          this->transmittanceLUT.bind(2);
          break;
        }
      }
    }
    else
    {
      this->skyboxParameters.x = 0;
      params0.x = this->roughness;
    }

    this->skyboxParamBuffer.setData(0, sizeof(glm::vec4), &(params0.x));
    this->skyboxParamBuffer.setData(1 * sizeof(glm::vec4), sizeof(glm::vec4), &(params1.x));
    this->skyboxParamBuffer.setData(2 * sizeof(glm::vec4), sizeof(glm::vec4), &(params2.x));
    this->skyboxParamBuffer.setData(3 * sizeof(glm::vec4), sizeof(glm::ivec4),
                                    &(this->skyboxParameters.x));

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
        auto& preethamSkyParams = this->getSkyParams<PreethamSkyParams>(DynamicSkyType::Preetham);

        auto preethamModel = glm::vec4(preethamSkyParams.turbidity, preethamSkyParams.sunSize,
                                       preethamSkyParams.sunIntensity, 0.0f);

        this->preethamParams.bindToPoint(1);
        this->preethamParams.setData(0, sizeof(glm::vec3), &preethamSkyParams.sunPos.x);
        this->preethamParams.setData(sizeof(glm::vec3), sizeof(float), &preethamSkyParams.turbidity);
        this->preethamParams.setData(sizeof(glm::vec4), sizeof(float), &preethamSkyParams.sunIntensity);
        this->preethamParams.setData(sizeof(glm::vec4) + sizeof(float), sizeof(float), &preethamSkyParams.sunSize);

        this->skyViewLUT.bindAsImage(0, 0, ImageAccessPolicy::Write);

        this->preethamLUTCompute.launchCompute(256 / 32, 128 / 32, 1);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

        break;
      }

      case DynamicSkyType::Hillaire:
      {
        // Updates the three LUTs associated with the Hillaire sky model.
        glm::vec4 params1 = glm::vec4(0.0f);
        glm::vec4 params2 = glm::vec4(0.0f);
        glm::vec4 params3 = glm::vec4(0.0f);
        glm::vec4 params4 = glm::vec4(0.0f);
        glm::vec4 params5 = glm::vec4(0.0f);

        auto& hillaireSkyParams = this->getSkyParams<HillaireSkyParams>(DynamicSkyType::Hillaire);

        params1.x = hillaireSkyParams.rayleighScatteringBase.x;
        params1.y = hillaireSkyParams.rayleighScatteringBase.y;
        params1.z = hillaireSkyParams.rayleighScatteringBase.z;
        params1.w = hillaireSkyParams.rayleighAbsorptionBase;

        params2.x = hillaireSkyParams.ozoneAbsorptionBase.x;
        params2.y = hillaireSkyParams.ozoneAbsorptionBase.y;
        params2.z = hillaireSkyParams.ozoneAbsorptionBase.z;
        params2.w = hillaireSkyParams.mieScatteringBase;

        params3.x = hillaireSkyParams.mieAbsorptionBase;
        params3.y = hillaireSkyParams.planetRadius;
        params3.z = hillaireSkyParams.atmosphereRadius;
        params3.w = hillaireSkyParams.viewPos.x;

        params4.x = -1.0f * hillaireSkyParams.sunPos.x;
        params4.y = -1.0f * hillaireSkyParams.sunPos.y;
        params4.z = -1.0f * hillaireSkyParams.sunPos.z;
        params4.w = hillaireSkyParams.viewPos.y;

        params5.x = hillaireSkyParams.viewPos.z;

        this->hillaireParams.bindToPoint(1);
        this->hillaireParams.setData(0, sizeof(glm::vec4), &(params1.x));
        this->hillaireParams.setData(1 * sizeof(glm::vec4), sizeof(glm::vec4), &(params2.x));
        this->hillaireParams.setData(2 * sizeof(glm::vec4), sizeof(glm::vec4), &(params3.x));
        this->hillaireParams.setData(3 * sizeof(glm::vec4), sizeof(glm::vec4), &(params4.x));
        this->hillaireParams.setData(4 * sizeof(glm::vec4), sizeof(glm::vec4), &(params5.x));

        this->transmittanceLUT.bindAsImage(0, 0, ImageAccessPolicy::Write);
        this->transmittanceCompute.launchCompute(256 / 32, 64 / 32, 1);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

        this->transmittanceLUT.bind(2);
        this->multiScatLUT.bindAsImage(0, 0, ImageAccessPolicy::Write);
        this->multiScatCompute.launchCompute(32 / 32, 32 / 32, 1);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

        this->transmittanceLUT.bind(2);
        this->multiScatLUT.bind(3);
        this->skyViewLUT.bindAsImage(0, 0, ImageAccessPolicy::Write);
        this->skyViewCompute.launchCompute(256 / 32, 128 / 32, 1);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
      }
    }
  }

  void 
  EnvironmentMap::setDynamicSkyIBL()
  {
    this->staticIBL = false;

    // Clear and resize the cubemap.
    TextureCubeMapParams params = TextureCubeMapParams();

    params.internal = TextureInternalFormats::RGBA16f;
    params.dataType = TextureDataType::Floats;

    this->irradiance.setSize(32, 32, 4);
    this->irradiance.setParams(params);
    this->irradiance.initNullTexture();

    this->specPrefilter.setSize(64, 64, 4);
    this->specPrefilter.setParams(params);
    this->specPrefilter.initNullTexture();
    this->specPrefilter.generateMips();
  }

  void 
  EnvironmentMap::setStaticIBL()
  {
    this->staticIBL = true;

    auto state = Renderer3D::getState();

    this->unloadComputedMaps();
    this->equiToCubeMap(true, state->skyboxWidth, state->skyboxWidth);
    this->precomputeIrradiance(state->irradianceWidth, state->irradianceWidth, true);
    this->precomputeSpecular(state->prefilterWidth, state->prefilterWidth, true);
  }

  // Getters.
  uint
  EnvironmentMap::getTexID(const MapType &type)
  {
    switch (type)
    {
      case MapType::Skybox:
        return this->skybox.getID();
      case MapType::Irradiance:
        return this->irradiance.getID();
      case MapType::Prefilter:
        return this->specPrefilter.getID();
      case MapType::DynamicSky:
        return this->skyViewLUT.getID();
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
  EnvironmentMap::equiToCubeMap(const bool &isHDR, const uint &width,
                                const uint &height)
  {
    if (this->erMap)
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
      this->skybox.setSize(width, height, 4);
      this->skybox.setParams(params);
      this->skybox.initNullTexture();

      // Bind the equirectangular map for reading by the compute shader.
      this->erMap->bindAsImage(0, 0, ImageAccessPolicy::Read);

      // Bind the irradiance map for writing to by the compute shader.
      this->skybox.bindAsImage(1, 0, true, 0, ImageAccessPolicy::ReadWrite);

      // Launch the compute shader.
      this->equiToCubeCompute.launchCompute(width / 32, height / 32, 6);

      this->skybox.generateMips();

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Converted the equirectangular map to a cubemap"
                                  " (elapsed time: " + std::to_string(elapsed.count())
                                  + " s).", true, true));
    }
  }

  // Generate the diffuse irradiance map.
  void
  EnvironmentMap::precomputeIrradiance(const uint &width, const uint &height,
                                       bool isHDR)
  {
    if (this->staticIBL)
    {
      // The resulting cubemap from the convolution process.
      TextureCubeMapParams params = TextureCubeMapParams();
      if (isHDR)
      {
          params.internal = TextureInternalFormats::RGBA16f;
          params.dataType = TextureDataType::Floats;
      }

      this->irradiance.setSize(width, height, 4);
      this->irradiance.setParams(params);
      this->irradiance.initNullTexture();

      if (this->erMap)
      {
        Logger* logs = Logger::getInstance();
        
        auto start = std::chrono::steady_clock::now();
        
        // Bind the skybox for reading by the compute shader.
        this->skybox.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Read);
        
        // Bind the irradiance map for writing to by the compute shader.
        this->irradiance.bindAsImage(1, 0, true, 0, ImageAccessPolicy::Write);
        
        // Launch the compute shader.
        this->diffIrradCompute.launchCompute(width / 32, height / 32, 6);
        
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        logs->logMessage(LogMessage("Convoluted environment map (elapsed time: "
            + std::to_string(elapsed.count()) + " s).", true,
            true));
      }
    }
    else
    {
      // Bind the sky view LUT for reading by the compute shader.
      this->skyViewLUT.bind(0);

      // Bind the parameters for the LUT.
      this->skyboxParamBuffer.bindToPoint(2);

      // Bind the irradiance map for writing to by the compute shader.
      this->irradiance.bindAsImage(1, 0, true, 0, ImageAccessPolicy::Write);

      // Launch the compute shader.
      this->skyDiffCompute.launchCompute(32 / 32, 32 / 32, 6);
    }
  }

  // Generate the specular map components. Computes the pre-filtered environment
  // map first, than integrates the BRDF and stores the result in an LUT.
  void
  EnvironmentMap::precomputeSpecular(const uint &width, const uint &height,
                                     bool isHDR)
  {
    if (this->staticIBL)
    {
      // The resulting cubemap from the environment pre-filter.
      TextureCubeMapParams params = TextureCubeMapParams();
      params.minFilter = TextureMinFilterParams::LinearMipMapLinear;
      if (isHDR)
      {
          params.internal = TextureInternalFormats::RGBA16f;
          params.dataType = TextureDataType::Floats;
      }

      this->specPrefilter.setSize(width, height, 4);
      this->specPrefilter.setParams(params);
      this->specPrefilter.initNullTexture();
      this->specPrefilter.generateMips();

      if (this->erMap)
      {
        Logger* logs = Logger::getInstance();
        auto state = Renderer3D::getState();
        
        //--------------------------------------------------------------------------
        // The pre-filtered environment map component.
        //--------------------------------------------------------------------------
        auto start = std::chrono::steady_clock::now();
        
        this->iblParams.bindToPoint(2);
        glm::vec4 params0 = glm::vec4(0.0f);
        glm::ivec4 params1 = glm::ivec4(0);
        
        // Bind the enviroment map which is to be prefiltered.
        this->skybox.bind(0);
        
        // Perform the pre-filter for each roughness level.
        for (uint i = 0; i < 5; i++)
        {
          // Compute the current mip levels.
          uint mipWidth  = (uint) ((float) width * std::pow(0.5f, i));
          uint mipHeight = (uint) ((float) height * std::pow(0.5f, i));
        
          // Bind the irradiance map for writing to by the compute shader.
          this->specPrefilter.bindAsImage(1, i, true, 0, ImageAccessPolicy::Write);
        
          // Roughness.
          params0.x = ((float) i) / ((float) (5 - 1));
          params1.x = static_cast<int>(state->prefilterSamples);
          this->iblParams.setData(0, sizeof(glm::vec4), &params0.x);
          this->iblParams.setData(sizeof(glm::vec4), sizeof(glm::ivec4), &params1.x);
        
          // Launch the compute.
          this->specIrradCompute.launchCompute(mipWidth / 32, mipHeight / 32, 6);
        }
        
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        logs->logMessage(LogMessage("Pre-filtered environment map (elapsed time: "
                                    + std::to_string(elapsed.count()) + " s).", true,
                                    true));
      }
    }
    else
    {
      this->iblParams.bindToPoint(2);
      glm::vec4 params0 = glm::vec4(0.0f);
      glm::ivec4 params1 = glm::ivec4(0);

      // Bind the enviroment map which is to be prefiltered.
      this->skyViewLUT.bind(0);
      // Bind the parameters for the LUT.
      this->skyboxParamBuffer.bindToPoint(3);

      // Perform the pre-filter for each roughness level.
      for (uint i = 0; i < 5; i++)
      {
        // Compute the current mip levels.
        uint mipWidth = (uint)((float)64.0f * std::pow(0.5f, i));
        uint mipHeight = (uint)((float)64.0f * std::pow(0.5f, i));

        // Bind the irradiance map for writing to by the compute shader.
        this->specPrefilter.bindAsImage(1, i, true, 0, ImageAccessPolicy::Write);

        // Roughness.
        params0.x = ((float)i) / ((float)(5 - 1));
        params1.x = static_cast<int>(512);
        this->iblParams.setData(0, sizeof(glm::vec4), &params0.x);
        this->iblParams.setData(sizeof(glm::vec4), sizeof(glm::ivec4), &params1.x);

        // Launch the compute.
        this->skySpecCompute.launchCompute(mipWidth / 32, mipHeight / 32, 6);
      }
    }
  }

  Shader*
  EnvironmentMap::getCubeProg()
  {
    return &this->dynamicSkyShader;
  }

  void
  EnvironmentMap::setDynamicSkyType(const DynamicSkyType &type)
  {
    this->currentDynamicSky = type;
    this->updateDynamicSky();
  }
}
