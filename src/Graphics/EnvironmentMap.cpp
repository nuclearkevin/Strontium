#include "Graphics/EnvironmentMap.h"

// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// Project includes.
#include "Core/Logs.h"
#include "Graphics/Buffers.h"
#include "Graphics/FrameBuffer.h"

namespace SciRenderer
{
  EnvironmentMap::EnvironmentMap(const char* vertPath, const char* fragPath,
                                 const char* cubeMeshPath)
    : erMap(nullptr)
    , skybox(nullptr)
    , irradiance(nullptr)
    , specPrefilter(nullptr)
    , brdfIntMap(nullptr)
    , gamma(2.2f)
    , exposure(1.0f)
  {
    this->cubeShader = new Shader(vertPath, fragPath);
    this->cube = new Mesh();
    this->cube->loadOBJFile(cubeMeshPath, false);
  }

  EnvironmentMap::~EnvironmentMap()
  {
    if (this->erMap != nullptr)
      Textures::deleteTexture(this->erMap);
    if (this->skybox != nullptr)
      Textures::deleteTexture(this->skybox);
    if (this->irradiance != nullptr)
      Textures::deleteTexture(this->irradiance);
    if (this->specPrefilter != nullptr)
      Textures::deleteTexture(this->specPrefilter);
    if (this->brdfIntMap != nullptr)
      Textures::deleteTexture(this->brdfIntMap);
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

  // Load a 2D equirectangular map, than convert it to a cubemap.
  // Assumes that the map is HDR by default.
  void
  EnvironmentMap::loadEquirectangularMap(const std::string &filepath,
                                         const Texture2DParams &params,
                                         const bool &isHDR,
                                         const GLuint &width, const GLuint &height)
  {
    Logger* logs = Logger::getInstance();

    // If we already have a cubemap of a given type, don't load another.
    if (this->skybox != nullptr && this->erMap != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has a skybox "
                                  "cubemap.", true, false, true));
      return;
    }

    auto start = std::chrono::steady_clock::now();

    Renderer3D* renderer = Renderer3D::getInstance();

    this->erMap = Textures::loadTexture2D(filepath, params, isHDR);

    // The framebuffer and renderbuffer for drawing the skybox to.
    GLuint equiFBO, equiRBO;

    // The resulting cubemap from the conversion process.
    this->skybox = new CubeMap();

    // A standard projection matrix, and 6 view matrices (one for each face of the
    // cubemap).
    glm::mat4 perspective = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[6] =
    {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f),
                  glm::vec3(0.0f,  0.0f,  1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
                  glm::vec3(0.0f,  0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // Generate and bind a cubemap texture.
    glGenTextures(1, &this->skybox->textureID);
    Textures::bindTexture(this->skybox);

    for (unsigned i = 0; i < 6; i++)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     width, height, 0, GL_RGB,
                     GL_FLOAT, nullptr);
      else
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                     width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Framebuffer to write the convoluted cubemap to.
    glGenFramebuffers(1, &equiFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, equiFBO);

    // Render attach for the framebuffer.
    glGenRenderbuffers(1, &equiRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, equiRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, equiRBO);

    Shader* eqToCube = new Shader("./res/shaders/pbr/equiconv.vs",
                                  "./res/shaders/pbr/equiconv.fs");

    eqToCube->addUniformSampler2D("equirectangularMap", 0);
    Textures::bindTexture(this->erMap, 0);

    glViewport(0, 0, width, height);

    // Perform the convolution for each face of the cubemap.
    for (unsigned i = 0; i < 6; i++)
    {
      eqToCube->addUniformMatrix("vP", perspective * views[i], GL_FALSE);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                             this->skybox->textureID, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (this->cube->hasVAO())
        renderer->draw(this->cube->getVAO(), eqToCube);
      else
      {
        this->cube->generateVAO(eqToCube);
        if (this->cube->hasVAO())
          renderer->draw(this->cube->getVAO(), eqToCube);
      }
    }

    // Unbind the framebuffer now that we're done with it.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Assign the texture sizes to the cubemap struct.
    for (unsigned i = 0; i < 6; i++)
    {
      this->skybox->width[i]  = width;
      this->skybox->height[i] = height;
      this->skybox->n[i]      = 3;
    }

    // Delete the frame and render buffers now that they've served their purpose.
    glDeleteFramebuffers(1, &equiFBO);
    glDeleteRenderbuffers(1, &equiRBO);

    // Delete the shader program used for the conversion as well.
    delete eqToCube;

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    logs->logMessage(LogMessage("Converted the equirectangular map to a cubemap"
                                " (elapsed time: " + std::to_string(elapsed.count())
                                + " s).", true, false, true));
  }

  // Bind/unbind a specific cubemap.
  void
  EnvironmentMap::bind(const MapType &type)
  {
    switch (type)
    {
      case MapType::SKYBOX:
        if (this->skybox != nullptr)
          Textures::bindTexture(this->skybox);
        break;
      case MapType::IRRADIANCE:
        if (this->irradiance != nullptr)
          Textures::bindTexture(this->irradiance);
        break;
      case MapType::PREFILTER:
        if (this->specPrefilter != nullptr)
          Textures::bindTexture(this->specPrefilter);
        break;
      case MapType::INTEGRATION:
        if (this->brdfIntMap != nullptr)
          Textures::bindTexture(this->brdfIntMap);
        break;
    }
  }

  void
  EnvironmentMap::unbind()
  {
    Textures::unbindTexture2D();
    Textures::unbindCubeMap();
  }

  void
  EnvironmentMap::bindToPoint(const MapType &type, GLuint bindPoint)
  {
    switch (type)
    {
      case MapType::SKYBOX:
        if (this->skybox != nullptr)
          Textures::bindTexture(this->skybox, bindPoint);
        break;
      case MapType::IRRADIANCE:
        if (this->irradiance != nullptr)
          Textures::bindTexture(this->irradiance, bindPoint);
        break;
      case MapType::PREFILTER:
        if (this->specPrefilter != nullptr)
          Textures::bindTexture(this->specPrefilter, bindPoint);
        break;
      case MapType::INTEGRATION:
        if (this->brdfIntMap != nullptr)
          Textures::bindTexture(this->brdfIntMap, bindPoint);
        break;
    }
  }

  // Draw the skybox.
  void
  EnvironmentMap::draw(Camera* camera)
  {
    Logger* logs = Logger::getInstance();

    if (this->skybox == nullptr)
    {
      return;
    }

    Renderer3D* renderer = Renderer3D::getInstance();

    glDepthFunc(GL_LEQUAL);

    glm::mat4 vP = camera->getProjMatrix() * glm::mat4(glm::mat3(camera->getViewMatrix()));

    this->cubeShader->bind();
    this->cubeShader->addUniformMatrix("vP", vP, GL_FALSE);
    this->cubeShader->addUniformSampler2D("skybox", 0);
    this->cubeShader->addUniformFloat("gamma", this->gamma);
    this->cubeShader->addUniformFloat("exposure", this->exposure);
    this->bindToPoint(MapType::SKYBOX, 0);

    if (this->cube->hasVAO())
      renderer->draw(this->cube->getVAO(), this->cubeShader);
    else
    {
      this->cube->generateVAO(this->cubeShader);
      if (this->cube->hasVAO())
        renderer->draw(this->cube->getVAO(), this->cubeShader);
    }

    Textures::unbindCubeMap();
    glDepthFunc(GL_LESS);
  }

  // Generate the diffuse irradiance map.
  void
  EnvironmentMap::precomputeIrradiance(GLuint width, GLuint height, bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    if (this->irradiance != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has an irradiance "
                                  "map.", true, false, true));
      return;
    }

    auto start = std::chrono::steady_clock::now();

    Renderer3D* renderer = Renderer3D::getInstance();

    // The framebuffer and renderbuffer for drawing the irradiance map to.
    GLuint convoFBO, convoRBO;

    // The resulting cubemap from the convolution process.
    this->irradiance = new CubeMap();

    // A standard projection matrix, and 6 view matrices (one for each face of the
    // cubemap).
    glm::mat4 perspective = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[6] =
    {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f),
                  glm::vec3(0.0f,  0.0f,  1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
                  glm::vec3(0.0f,  0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // Generate and bind a cubemap textue to slot 0.
    glGenTextures(1, &this->irradiance->textureID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradiance->textureID);

    for (unsigned i = 0; i < 6; i++)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                   width, height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Framebuffer to write the convoluted cubemap to.
    glGenFramebuffers(1, &convoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, convoFBO);

    // Render attach for the framebuffer.
    glGenRenderbuffers(1, &convoRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, convoRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, convoRBO);

    // The convolution program.
    Shader* convoProg = new Shader("./res/shaders/pbr/diffuseConv.vs",
                                   "./res/shaders/pbr/diffuseConv.fs");
    convoProg->addUniformSampler2D("environmentMap", 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textureID);

    glViewport(0, 0, width, height);

    // Perform the convolution for each face of the cubemap.
    for (unsigned i = 0; i < 6; i++)
    {
      convoProg->addUniformMatrix("vP", perspective * views[i], GL_FALSE);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                             this->irradiance->textureID, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (this->cube->hasVAO())
        renderer->draw(this->cube->getVAO(), convoProg);
      else
      {
        this->cube->generateVAO(convoProg);
        if (this->cube->hasVAO())
          renderer->draw(this->cube->getVAO(), convoProg);
      }
    }
    // Unbind the framebuffer now that we're done with it.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Assign the texture sizes to the cubemap struct.
    for (unsigned i = 0; i < 6; i++)
    {
      this->irradiance->width[i]  = width;
      this->irradiance->height[i] = height;
      this->irradiance->n[i]      = 3;
    }

    // Delete the frame and render buffers now that they've served their purpose.
    glDeleteFramebuffers(1, &convoFBO);
    glDeleteRenderbuffers(1, &convoRBO);

    // Delete the shader program used for the convolution as well.
    delete convoProg;

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    logs->logMessage(LogMessage("Convoluted environment map (elapsed time: "
                                + std::to_string(elapsed.count()) + " s).", true,
                                false, true));
  }

  // Generate the specular map components. Computes the pre-filtered environment
  // map first, than the integration map second.
  void
  EnvironmentMap::precomputeSpecular(GLuint width, GLuint height, bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    if (this->specPrefilter != nullptr && this->brdfIntMap != nullptr)
    {
      logs->logMessage(LogMessage("This environment map already has the required "
                                  "specular map and lookup texture.",
                                  true, false, true));
      return;
    }

    Renderer3D* renderer = Renderer3D::getInstance();

    // The framebuffer and renderbuffer for drawing the pre-filter and
    // integration textures.
    GLuint specFBO, specRBO;

    // Framebuffer to write the pre-filtered environment map to.
    glGenFramebuffers(1, &specFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, specFBO);

    // Render attach for the framebuffer.
    glGenRenderbuffers(1, &specRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, specRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, specRBO);

    // A standard projection matrix, and 6 view matrices (one for each face of the
    // cubemap).
    glm::mat4 perspective = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[6] =
    {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f),
                  glm::vec3(0.0f,  0.0f,  1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
                  glm::vec3(0.0f,  0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f,  0.0f))
    };

    if (this->specPrefilter == nullptr)
    {
      //--------------------------------------------------------------------------
      // The pre-filtered environment map component. This is hella slow,
      // need to move it to compute shaders.
      //--------------------------------------------------------------------------
      auto start = std::chrono::steady_clock::now();

      // The resulting cubemap from the environment pre-filter.
      this->specPrefilter = new CubeMap();

      // Generate the pre-filter texture.
      glGenTextures(1, &this->specPrefilter->textureID);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->specPrefilter->textureID);

      for (unsigned i = 0; i < 6; i++)
      {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                     width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, nullptr);
      }

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

      // Compile and link the shader program used to generate the pre-filter map.
      Shader* specPrefilterProg = new Shader("res/shaders/pbr/specPrefilter.vs",
                                             "res/shaders/pbr/specPrefilter.fs");
      specPrefilterProg->addUniformSampler2D("environmentMap", 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textureID);

      // Perform the pre-filter for each fact of the cubemap.
      for (GLuint i = 0; i < 5; i++)
      {
        // Compute the current mip levels.
        GLuint mipWidth  = (GLuint) ((float) width * std::pow(0.5f, i));
        GLuint mipHeight = (GLuint) ((float) height * std::pow(0.5f, i));

        // Update the framebuffer to those levels.
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = ((float) i) / ((float) (5 - 1));
        specPrefilterProg->addUniformFloat("roughness", roughness);

        for (GLuint j = 0; j < 6; j++)
        {
          specPrefilterProg->addUniformMatrix("vP", perspective * views[j], GL_FALSE);
          specPrefilterProg->addUniformFloat("resolution", this->skybox->width[j]);
          glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_CUBE_MAP_POSITIVE_X + j,
                                 this->specPrefilter->textureID, i);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          if (this->cube->hasVAO())
            renderer->draw(this->cube->getVAO(), specPrefilterProg);
          else
          {
            this->cube->generateVAO(specPrefilterProg);
            if (this->cube->hasVAO())
              renderer->draw(this->cube->getVAO(), specPrefilterProg);
          }
        }
      }

      // Unbind the framebuffer now that we're done with it.
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      // Assign the texture sizes to the cubemap struct.
      for (unsigned i = 0; i < 6; i++)
      {
        this->specPrefilter->width[i]  = width;
        this->specPrefilter->height[i] = height;
        this->specPrefilter->n[i]      = 3;
      }

      // Delete the shader program used for the pre-filter.
      delete specPrefilterProg;

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

      this->brdfIntMap = new Texture2D();

      glGenTextures(1, &this->brdfIntMap->textureID);
      glBindTexture(GL_TEXTURE_2D, this->brdfIntMap->textureID);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width, height, 0, GL_RG,
                   GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glBindFramebuffer(GL_FRAMEBUFFER, specFBO);
      glBindRenderbuffer(GL_RENDERBUFFER, specRBO);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             this->brdfIntMap->textureID, 0);

      // Shader program to compute the integration lookup texture.
      Shader* intProg = new Shader("./res/shaders/pbr/brdfIntegration.vs",
    												       "./res/shaders/pbr/brdfIntegration.fs");

      glViewport(0, 0, width, height);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      renderer->drawFSQ(intProg);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      // Delete the shader object, we're done with it.
      delete intProg;

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      logs->logMessage(LogMessage("Generated BRDF lookup texture (elapsed time: "
                                  + std::to_string(elapsed.count()) + " s).", true,
                                  false, true));
    }

    // Delete the frame and render buffers now that they've served their purpose.
    glDeleteFramebuffers(1, &specFBO);
    glDeleteRenderbuffers(1, &specRBO);
  }
}
