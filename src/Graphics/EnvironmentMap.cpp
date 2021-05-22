#include "Graphics/EnvironmentMap.h"

// Project includes.
#include "Core/Logs.h"
#include "Graphics/Buffers.h"
#include "Graphics/Compute.h"

namespace SciRenderer
{
  EnvironmentMap::EnvironmentMap(const char* vertPath, const char* fragPath,
                                 const char* cubeMeshPath)
    : erMap(nullptr)
    , skybox(nullptr)
    , irradiance(nullptr)
    , specPrefilter(nullptr)
    , brdfIntMap(nullptr)
    , currentEnvironment(MapType::Skybox)
    , gamma(2.2f)
    , roughness(0.0f)
    , exposure(1.0f)
  {
    this->cubeShader = createShared<Shader>(vertPath, fragPath);
    this->cube = createShared<Mesh>();
    this->cube->loadOBJFile(cubeMeshPath, false);
  }

  EnvironmentMap::~EnvironmentMap()
  {
    this->unloadEnvironment();
  }

  // Load 6 textures from a file to generate a cubemap.
  void
  EnvironmentMap::loadCubeMap(const std::vector<std::string> &filenames,
                              const TextureCubeMapParams &params,
                              const bool &isHDR)
  {
    Logger* logs = Logger::getInstance();

    // If we already have a cubemap of a given type, don't load another.
    if (this->skybox != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has a skybox "
                                  "cubemap.", true, false, true));
      return;
    }
    this->skybox = Textures::loadTextureCubeMap(filenames, params, isHDR);
  }

  // Unload all the textures associated from this environment.
  void
  EnvironmentMap::unloadEnvironment()
  {
    if (this->erMap != nullptr)
    {
      Textures::deleteTexture(this->erMap);
      this->erMap = Shared<Texture2D>(nullptr);
    }
    if (this->skybox != nullptr)
    {
      Textures::deleteTexture(this->skybox);
      this->skybox = Shared<CubeMap>(nullptr);
    }
    if (this->irradiance != nullptr)
    {
      Textures::deleteTexture(this->irradiance);
      this->irradiance = Shared<CubeMap>(nullptr);
    }
    if (this->specPrefilter != nullptr)
    {
      Textures::deleteTexture(this->specPrefilter);
      this->specPrefilter = Shared<CubeMap>(nullptr);
    }
    if (this->brdfIntMap != nullptr)
    {
      Textures::deleteTexture(this->brdfIntMap);
      this->brdfIntMap = Shared<Texture2D>(nullptr);
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
          Textures::bindTexture(this->erMap);
        break;
      case MapType::Skybox:
        if (this->skybox != nullptr)
          Textures::bindTexture(this->skybox);
        break;
      case MapType::Irradiance:
        if (this->irradiance != nullptr)
          Textures::bindTexture(this->irradiance);
        break;
      case MapType::Prefilter:
        if (this->specPrefilter != nullptr)
          Textures::bindTexture(this->specPrefilter);
        break;
      case MapType::Integration:
        if (this->brdfIntMap != nullptr)
          Textures::bindTexture(this->brdfIntMap);
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
          Textures::bindTexture(this->erMap, bindPoint);
        break;
      case MapType::Skybox:
        if (this->skybox != nullptr)
          Textures::bindTexture(this->skybox, bindPoint);
        break;
      case MapType::Irradiance:
        if (this->irradiance != nullptr)
          Textures::bindTexture(this->irradiance, bindPoint);
        break;
      case MapType::Prefilter:
        if (this->specPrefilter != nullptr)
          Textures::bindTexture(this->specPrefilter, bindPoint);
        break;
      case MapType::Integration:
        if (this->brdfIntMap != nullptr)
          Textures::bindTexture(this->brdfIntMap, bindPoint);
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
    this->cubeShader->addUniformSampler2D("skybox", 0);
    this->cubeShader->addUniformFloat("gamma", this->gamma);
    this->cubeShader->addUniformFloat("exposure", this->exposure);

    if (this->currentEnvironment == MapType::Prefilter)
      this->cubeShader->addUniformFloat("roughness", this->roughness);

    this->bind(this->currentEnvironment, 0);

    if (!this->cube->hasVAO())
      this->cube->generateVAO(this->cubeShader);
  }

  // Getters.
  GLuint
  EnvironmentMap::getTexID(const MapType &type)
  {
    switch (type)
    {
      case MapType::Equirectangular:
        if (this->erMap != nullptr)
          return this->erMap->textureID;
      case MapType::Skybox:
        if (this->skybox != nullptr)
          return this->skybox->textureID;
      case MapType::Irradiance:
        if (this->irradiance != nullptr)
          return this->irradiance->textureID;
      case MapType::Prefilter:
        if (this->specPrefilter != nullptr)
          return this->specPrefilter->textureID;
      case MapType::Integration:
        if (this->brdfIntMap != nullptr)
          return this->brdfIntMap->textureID;
    }

    return 0;
  }

  // Load a 2D equirectangular map.
  void
  EnvironmentMap::loadEquirectangularMap(const std::string &filepath,
                                         const Texture2DParams &params,
                                         const bool &isHDR)
  {
    Logger* logs = Logger::getInstance();

    // If we already have an equirectangular map, don't load another.
    if (this->erMap != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has an "
                                  "equirectangular map.", true, false, true));
      return;
    }

    this->erMap = Textures::loadTexture2D(filepath, params, isHDR);
  }

  // Convert the loaded equirectangular map to a cubemap.
  void
  EnvironmentMap::equiToCubeMap(const bool &isHDR, const GLuint &width,
                                const GLuint &height)
  {
    Logger* logs = Logger::getInstance();

    // If we already have a cubemap of a given type, don't load another.
    if (this->skybox != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has a skybox "
                                  "cubemap.", true, false, true));
      return;
    }
    if (this->erMap == nullptr)
    {
      logs->logMessage(LogMessage("No equirectangular map attached! Cannot "
                                  "generate a cubemap.", true, false, true));
      return;
    }
    auto start = std::chrono::steady_clock::now();

    // The resulting cubemap from the conversion process.
    this->skybox = createShared<CubeMap>();

    // Generate and bind a cubemap texture.
    glGenTextures(1, &this->skybox->textureID);
    Textures::bindTexture(this->skybox);

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
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // The equirectangular to cubemap compute shader.
    ComputeShader conversionShader = ComputeShader("./res/shaders/compute/equiConversion.cs");

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
    glBindImageTexture(0, this->erMap->textureID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

    // Bind the irradiance map for writing to by the compute shader.
    glBindImageTexture(1, this->skybox->textureID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

    // Launch the compute shader.
    conversionShader.bind();
    conversionShader.launchCompute(glm::ivec3(width / 32, height / 32, 6));

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;


    logs->logMessage(LogMessage("Converted the equirectangular map to a cubemap"
                                " (elapsed time: " + std::to_string(elapsed.count())
                                + " s).", true, false, true));
  }

  // Generate the diffuse irradiance map.
  void
  EnvironmentMap::precomputeIrradiance(const GLuint &width, const GLuint &height,
                                       bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    if (this->irradiance != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has an irradiance "
                                  "map.", true, false, true));
      return;
    }
    auto start = std::chrono::steady_clock::now();

    // The resulting cubemap from the convolution process.
    this->irradiance = createShared<CubeMap>();

    // Assign the texture sizes to the cubemap struct.
    for (unsigned i = 0; i < 6; i++)
    {
      this->irradiance->width[i]  = width;
      this->irradiance->height[i] = height;
      this->irradiance->n[i]      = 3;
    }

    // Generate and bind a cubemap textue to slot 0.
    glGenTextures(1, &this->irradiance->textureID);
    Textures::bindTexture(this->irradiance);

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
    ComputeShader convolutionShader = ComputeShader("./res/shaders/compute/diffuseConv.cs");

    // Buffer to pass the sizes of the image to the compute shader.
    glm::vec2 dimensions = glm::vec2((GLfloat) width, (GLfloat) height);
    ShaderStorageBuffer sizeBuff = ShaderStorageBuffer(&dimensions,
                                                       2 * sizeof(GLfloat),
                                                       BufferType::Static);
    sizeBuff.bindToPoint(2);

    // Bind the skybox for reading by the compute shader.
    glBindImageTexture(0, this->skybox->textureID, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);

    // Bind the irradiance map for writing to by the compute shader.
    glBindImageTexture(1, this->irradiance->textureID, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    // Launch the compute shader.
    convolutionShader.bind();
    convolutionShader.launchCompute(glm::ivec3(width / 32, height / 32, 6));

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    logs->logMessage(LogMessage("Convoluted environment map (elapsed time: "
                                + std::to_string(elapsed.count()) + " s).", true,
                                false, true));
  }

  // Generate the specular map components. Computes the pre-filtered environment
  // map first, than integrates the BRDF and stores the result in an LUT.
  void
  EnvironmentMap::precomputeSpecular(const GLuint &width, const GLuint &height,
                                     bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    if (this->specPrefilter != nullptr && this->brdfIntMap != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has the required "
                                  "specular map and lookup texture.",
                                  true, false, true));
      return;
    }

    if (this->specPrefilter == nullptr)
    {
      //--------------------------------------------------------------------------
      // The pre-filtered environment map component.
      //--------------------------------------------------------------------------
      auto start = std::chrono::steady_clock::now();

      // The resulting cubemap from the environment pre-filter.
      this->specPrefilter = createShared<CubeMap>();

      // Generate the pre-filter texture.
      glGenTextures(1, &this->specPrefilter->textureID);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->specPrefilter->textureID);

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
      ComputeShader filterShader = ComputeShader("./res/shaders/compute/specPrefilter.cs");

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
      Textures::bindTexture(this->skybox, 0);

      // Perform the pre-filter for each roughness level.
      for (GLuint i = 0; i < 5; i++)
      {
        // Compute the current mip levels.
        GLuint mipWidth  = (GLuint) ((GLfloat) width * std::pow(0.5f, i));
        GLuint mipHeight = (GLuint) ((GLfloat) height * std::pow(0.5f, i));

        float roughness = ((float) i) / ((float) (5 - 1));

        // Bind the irradiance map for writing to by the compute shader.
        glBindImageTexture(1, this->specPrefilter->textureID, i, GL_TRUE, 0,
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
                                  false, true));
    }

    if (this->brdfIntMap == nullptr)
    {
      //--------------------------------------------------------------------------
      // The BRDF integration map component.
      //--------------------------------------------------------------------------
      auto start = std::chrono::steady_clock::now();

      this->brdfIntMap = createShared<Texture2D>();
      this->brdfIntMap->width = 512;
      this->brdfIntMap->height = 512;

      glGenTextures(1, &this->brdfIntMap->textureID);
      glBindTexture(GL_TEXTURE_2D, this->brdfIntMap->textureID);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 512, 512, 0,
                   GL_RGBA, GL_FLOAT, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      ComputeShader integrateBRDF = ComputeShader("./res/shaders/compute/integrateBRDF.cs");

      // Bind the irradiance map for writing to by the compute shader.
      glBindImageTexture(3, this->brdfIntMap->textureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

      // Launch the compute shader.
      integrateBRDF.bind();
      integrateBRDF.launchCompute(glm::ivec3(512 / 32, 512 / 32, 1));

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Generated BRDF lookup texture (elapsed time: "
                                  + std::to_string(elapsed.count()) + " s).", true,
                                  false, true));
    }
  }
}
