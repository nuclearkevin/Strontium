#include "Graphics/EnvironmentMap.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/AssetManager.h"
#include "Graphics/Buffers.h"
#include "Graphics/Compute.h"

namespace Strontium
{
  EnvironmentMap::EnvironmentMap()
    : erMap(nullptr)
    , skybox(nullptr)
    , irradiance(nullptr)
    , specPrefilter(nullptr)
    , brdfIntMap(nullptr)
    , currentEnvironment(MapType::Skybox)
    , intensity(1.0f)
    , roughness(0.0f)
    , filepath("")
  {
    auto modelManager = AssetManager<Model>::getManager();
    auto shaderCache = AssetManager<Shader>::getManager();

    this->cubeShader = new Shader("./assets/shaders/forward/pbr/pbrSkybox.vs", "./assets/shaders/forward/pbr/pbrSkybox.fs");
    shaderCache->attachAsset("skybox_shader", this->cubeShader);

    this->cube.loadModel("./assets/.internal/cube.fbx");
  }

  EnvironmentMap::~EnvironmentMap()
  {
    this->unloadEnvironment();
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
    if (this->brdfIntMap != nullptr)
    {
      this->brdfIntMap = Unique<Texture2D>(nullptr);
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
    if (this->brdfIntMap != nullptr)
    {
      this->brdfIntMap = Unique<Texture2D>(nullptr);
    }
  }

  // Bind/unbind a specific cubemap.
  void
  EnvironmentMap::bind(const MapType &type)
  {
    switch (type)
    {
      case MapType::Equirectangular:
        if (this->erMap != nullptr)
          this->erMap->bind();
        break;
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
      case MapType::Integration:
        if (this->brdfIntMap != nullptr)
          this->brdfIntMap->bind();
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
      case MapType::Equirectangular:
        if (this->erMap != nullptr)
          this->erMap->bind(bindPoint);
        break;
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
      case MapType::Integration:
        if (this->brdfIntMap != nullptr)
          this->brdfIntMap->bind(bindPoint);
        break;
    }
  }

  // Draw the skybox.
  void
  EnvironmentMap::configure(Shared<Camera> camera)
  {
    glm::mat4 vP = camera->getProjMatrix() * glm::mat4(glm::mat3(camera->getViewMatrix()));

    this->cubeShader->bind();
    this->cubeShader->addUniformMatrix("vP", vP, GL_FALSE);
    this->cubeShader->addUniformSampler("skybox", 0);

    if (this->currentEnvironment == MapType::Prefilter)
      this->cubeShader->addUniformFloat("roughness", this->roughness);

    this->bind(this->currentEnvironment, 0);

    for (auto& submesh : this->cube.getSubmeshes())
    {
      if (!submesh.hasVAO())
        submesh.generateVAO();
    }
  }

  // Getters.
  GLuint
  EnvironmentMap::getTexID(const MapType &type)
  {
    switch (type)
    {
      case MapType::Equirectangular:
        if (this->erMap != nullptr)
          return this->erMap->getID();
      case MapType::Skybox:
        if (this->skybox != nullptr)
          return this->skybox->getID();
      case MapType::Irradiance:
        if (this->irradiance != nullptr)
          return this->irradiance->getID();
      case MapType::Prefilter:
        if (this->specPrefilter != nullptr)
          return this->specPrefilter->getID();
      case MapType::Integration:
        if (this->brdfIntMap != nullptr)
          return this->brdfIntMap->getID();
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

    // The equirectangular to cubemap compute shader.
    ComputeShader conversionShader = ComputeShader("./assets/shaders/compute/equiConversion.cs");

    // Buffer to pass the sizes of the image to the compute shader.
    struct
    {
      glm::vec2 equiSize;
      glm::vec2 enviSize;
    } dims;
    dims.equiSize = glm::vec2(this->erMap->width, this->erMap->height);
    dims.enviSize = glm::vec2(width, height);
    ShaderStorageBuffer sizeBuff = ShaderStorageBuffer(&dims, sizeof(dims),
                                                       BufferType::Static);
    sizeBuff.bindToPoint(2);

    // Bind the equirectangular map for reading by the compute shader.
    this->erMap->bindAsImage(0, 0, ImageAccessPolicy::Read);

    // Bind the irradiance map for writing to by the compute shader.
    this->skybox->bindAsImage(1, 0, true, 0, ImageAccessPolicy::ReadWrite);

    // Launch the compute shader.
    conversionShader.launchCompute(glm::ivec3(width / 32, height / 32, 6));

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

    // Convolution compute shader.
    ComputeShader convolutionShader = ComputeShader("./assets/shaders/compute/diffuseConv.cs");

    // Buffer to pass the sizes of the image to the compute shader.
    glm::vec2 dimensions = glm::vec2((GLfloat) width, (GLfloat) height);
    ShaderStorageBuffer sizeBuff = ShaderStorageBuffer(&dimensions,
                                                       2 * sizeof(GLfloat),
                                                       BufferType::Static);
    sizeBuff.bindToPoint(2);

    // Bind the skybox for reading by the compute shader.
    this->skybox->bindAsImage(0, 0, true, 0, ImageAccessPolicy::Read);

    // Bind the irradiance map for writing to by the compute shader.
    this->irradiance->bindAsImage(1, 0, true, 0, ImageAccessPolicy::Write);

    // Launch the compute shader.
    convolutionShader.launchCompute(glm::ivec3(width / 32, height / 32, 6));

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

      // The specular prefilter compute shader.
      ComputeShader filterShader = ComputeShader("./assets/shaders/compute/specPrefilter.cs");

      // A buffer with the parameters required for computing the prefilter.
      struct
      {
        glm::vec2 enviSize;
        glm::vec2 mipSize;
        GLfloat roughness;
        GLfloat resolution;
      } temp;
      ShaderStorageBuffer paramBuff = ShaderStorageBuffer(sizeof(temp), BufferType::Static);

      // Bind the enviroment map which is to be prefiltered.
      this->skybox->bind(0);

      // Perform the pre-filter for each roughness level.
      for (GLuint i = 0; i < 5; i++)
      {
        // Compute the current mip levels.
        GLuint mipWidth  = (GLuint) ((GLfloat) width * std::pow(0.5f, i));
        GLuint mipHeight = (GLuint) ((GLfloat) height * std::pow(0.5f, i));

        GLfloat roughness = ((GLfloat) i) / ((GLfloat) (5 - 1));

        // Bind the irradiance map for writing to by the compute shader.
        this->specPrefilter->bindAsImage(1, i, true, 0, ImageAccessPolicy::Write);

        temp.enviSize = glm::vec2((GLfloat) this->skybox->width[0],
                                    (GLfloat) this->skybox->height[0]);
        temp.mipSize = glm::vec2((GLfloat) mipWidth, (GLfloat) mipHeight);
        temp.roughness = roughness;
        temp.resolution = this->skybox->width[0];
        paramBuff.setData(0, sizeof(temp), &temp);
        paramBuff.bindToPoint(2);

        // Launch the compute.
        filterShader.launchCompute(glm::ivec3(mipWidth / 32, mipHeight / 32, 6));
      }

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Pre-filtered environment map (elapsed time: "
                                  + std::to_string(elapsed.count()) + " s).", true,
                                  true));
    }

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

      ComputeShader integrateBRDF = ComputeShader("./assets/shaders/compute/integrateBRDF.cs");

      // Bind the integration map for writing to by the compute shader.
      this->brdfIntMap->bindAsImage(3, 0, ImageAccessPolicy::Write);

      // Launch the compute shader.
      integrateBRDF.launchCompute(glm::ivec3(512 / 32, 512 / 32, 1));

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Generated BRDF lookup texture (elapsed time: "
                                  + std::to_string(elapsed.count()) + " s).", true,
                                  true));
    }
  }
}
