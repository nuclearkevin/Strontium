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
    this->skybox = createUnique<CubeMap>();

    // Assign the texture sizes to the cubemap struct.
    for (unsigned i = 0; i < 6; i++)
    {
      this->skybox->width[i]  = width;
      this->skybox->height[i] = height;
      this->skybox->n[i]      = 3;
    }

    for (unsigned i = 0; i < 6; i++)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F,
                     width, height, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
      else
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
                     width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    glBindImageTexture(0, this->erMap->getID(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

    // Bind the irradiance map for writing to by the compute shader.
    glBindImageTexture(1, this->skybox->getID(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

    // Launch the compute shader.
    conversionShader.bind();
    conversionShader.launchCompute(glm::ivec3(width / 32, height / 32, 6));

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;


    logs->logMessage(LogMessage("Converted the equirectangular map to a cubemap"
                                " (elapsed time: " + std::to_string(elapsed.count())
                                + " s).", true, true));
    this->skybox->bind();
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  }

  // Generate the diffuse irradiance map.
  void
  EnvironmentMap::precomputeIrradiance(const GLuint &width, const GLuint &height,
                                       bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    auto start = std::chrono::steady_clock::now();

    // The resulting cubemap from the convolution process.
    this->irradiance = createUnique<CubeMap>();

    // Assign the texture sizes to the cubemap struct.
    for (unsigned i = 0; i < 6; i++)
    {
      this->irradiance->width[i]  = width;
      this->irradiance->height[i] = height;
      this->irradiance->n[i]      = 3;
    }

    for (unsigned i = 0; i < 6; i++)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F,
                     width, height, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
      else
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
                     width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Convolution compute shader.
    ComputeShader convolutionShader = ComputeShader("./assets/shaders/compute/diffuseConv.cs");

    // Buffer to pass the sizes of the image to the compute shader.
    glm::vec2 dimensions = glm::vec2((GLfloat) width, (GLfloat) height);
    ShaderStorageBuffer sizeBuff = ShaderStorageBuffer(&dimensions,
                                                       2 * sizeof(GLfloat),
                                                       BufferType::Static);
    sizeBuff.bindToPoint(2);

    // Bind the skybox for reading by the compute shader.
    glBindImageTexture(0, this->skybox->getID(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);

    // Bind the irradiance map for writing to by the compute shader.
    glBindImageTexture(1, this->irradiance->getID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    // Launch the compute shader.
    convolutionShader.bind();
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
      this->specPrefilter = createUnique<CubeMap>();

      for (unsigned i = 0; i < 6; i++)
      {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F,
                     width, height, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
        this->specPrefilter->width[i]  = width;
        this->specPrefilter->height[i] = height;
        this->specPrefilter->n[i]      = 3;
      }

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

      // The specular prefilter compute shader.
      ComputeShader filterShader = ComputeShader("./assets/shaders/compute/specPrefilter.cs");

      // A buffer with the parameters required for computing the prefilter.
      struct
      {
        glm::vec2 enviSize;
        glm::vec2 mipSize;
        GLfloat roughness;
        GLfloat resolution;
      } params;
      ShaderStorageBuffer paramBuff = ShaderStorageBuffer(sizeof(params),
                                                          BufferType::Static);

      // Bind the enviroment map which is to be prefiltered.
      this->skybox->bind(0);

      // Perform the pre-filter for each roughness level.
      for (GLuint i = 0; i < 5; i++)
      {
        // Compute the current mip levels.
        GLuint mipWidth  = (GLuint) ((GLfloat) width * std::pow(0.5f, i));
        GLuint mipHeight = (GLuint) ((GLfloat) height * std::pow(0.5f, i));

        float roughness = ((float) i) / ((float) (5 - 1));

        // Bind the irradiance map for writing to by the compute shader.
        glBindImageTexture(1, this->specPrefilter->getID(), i, GL_TRUE, 0,
                           GL_WRITE_ONLY, GL_RGBA16F);

        params.enviSize = glm::vec2((GLfloat) this->skybox->width[0],
                                    (GLfloat) this->skybox->height[0]);
        params.mipSize = glm::vec2((GLfloat) mipWidth, (GLfloat) mipHeight);
        params.roughness = roughness;
        params.resolution = this->skybox->width[0];
        paramBuff.setData(0, sizeof(params), &params);
        paramBuff.bindToPoint(2);

        // Launch the compute.
        filterShader.bind();
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

      this->brdfIntMap = createUnique<Texture2D>();
      this->brdfIntMap->width = 512;
      this->brdfIntMap->height = 512;

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 512, 512, 0,
                   GL_RGBA, GL_FLOAT, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      ComputeShader integrateBRDF = ComputeShader("./assets/shaders/compute/integrateBRDF.cs");

      // Bind the integration map for writing to by the compute shader.
      glBindImageTexture(3, this->brdfIntMap->getID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

      // Launch the compute shader.
      integrateBRDF.bind();
      integrateBRDF.launchCompute(glm::ivec3(512 / 32, 512 / 32, 1));

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Generated BRDF lookup texture (elapsed time: "
                                  + std::to_string(elapsed.count()) + " s).", true,
                                  true));
    }
  }
}
