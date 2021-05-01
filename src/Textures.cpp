#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// Project includes.
#include "Textures.h"
#include "Logs.h"

using namespace SciRenderer;

//------------------------------------------------------------------------------
// 2D texture controller here.
//------------------------------------------------------------------------------
// Frees all the textures from GPU memory when deleted.
Texture2DController::~Texture2DController()
{
  for (auto& name : this->texNames)
  {
    Texture2D* temp = this->textures[name];
    glDeleteTextures(1, &temp->textureID);
  }
  this->textures.clear();
  this->texNames.clear();
}

// Load a texture from a file into the texture handler.
void
Texture2DController::loadFomFile(const char* filepath, const std::string &texName)
{
  // Struct for the texture information.
  Texture2D* newTex = new Texture2D();

  // Generate and bind the GPU texture.
  glGenTextures(1, &newTex->textureID);
  glBindTexture(GL_TEXTURE_2D, newTex->textureID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Load the texture.
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load(filepath, &newTex->width, &newTex->height,
                                  &newTex->n, 0);

  // Something went wrong while loading, abort.
  if (!data)
  {
    std::cout << "Failed to load image at: \"" << std::string(filepath) << "\""
              << std::endl;
    stbi_image_free(data);
    glDeleteTextures(1, &newTex->textureID);
    return;
  }

  // Generate a 2D texture.
  if (newTex->n == 1)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, newTex->width, newTex->height, 0,
                 GL_RED, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  else if (newTex->n == 2)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, newTex->width, newTex->height, 0,
                 GL_RG, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  else if (newTex->n == 3)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, newTex->width, newTex->height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  else if (newTex->n == 4)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newTex->width, newTex->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  // Push the texture to an unordered map.
  this->textures.insert({ texName, newTex });
  this->texNames.push_back(texName);

  // Free memory.
  stbi_image_free(data);
}

// Deletes a texture given its name.
void
Texture2DController::deleteTexture(const std::string &texName)
{
  this->textures.erase(texName);
}

// Bind/unbind a texture.
void
Texture2DController::bind(const std::string &texName)
{
  glBindTexture(GL_TEXTURE_2D, this->textures.at(texName)->textureID);
}

void
Texture2DController::unbind()
{
  glBindTexture(GL_TEXTURE_2D, 0);
}

// Binds a texture to a point.
void
Texture2DController::bindToPoint(const std::string &texName, GLuint bindPoint)
{
  glActiveTexture(GL_TEXTURE0 + bindPoint);
  glBindTexture(GL_TEXTURE_2D, this->textures.at(texName)->textureID);
}

// Get a texture ID.
GLuint
Texture2DController::getTexID(const std::string &texName)
{
  return this->textures.at(texName)->textureID;
}

//------------------------------------------------------------------------------
// Environment map here.
//------------------------------------------------------------------------------
EnvironmentMap::EnvironmentMap(const char* vertPath, const char* fragPath,
                               const char* cubeMeshPath)
  : skybox(nullptr)
  , irradiance(nullptr)
  , specPrefilter(nullptr)
  , brdfIntMap(nullptr)
{
  this->cubeShader = new Shader(vertPath, fragPath);
  this->cube = new Mesh();
  this->cube->loadOBJFile(cubeMeshPath, false);
}

EnvironmentMap::~EnvironmentMap()
{
  if (this->skybox != nullptr)
  {
    glDeleteTextures(1, &this->skybox->textureID);
    delete this->skybox;
  }
  if (this->irradiance)
  {
    glDeleteTextures(1, &this->irradiance->textureID);
    delete this->irradiance;
  }
  if (this->specPrefilter != nullptr)
  {
    glDeleteTextures(1, &this->specPrefilter->textureID);
    delete this->specPrefilter;
  }
  if (this->brdfIntMap != nullptr)
  {
    glDeleteTextures(1, &this->brdfIntMap->textureID);
    delete this->brdfIntMap;
  }
}

// Load 6 textures from a file to generate a cubemap.
void
EnvironmentMap::loadCubeMap(const std::vector<std::string> &filenames,
                            const MapType &type)
{
  // If there aren't enough faces, don't load.
  if (filenames.size() != 6)
  {
    std::cout << "Not enough faces to generate a cubemap, aborting." << std::endl;
    return;
  }

  // If we already have a cubemap of a given type, don't load another.
  if (type == SKYBOX && this->skybox != nullptr)
  {
    std::cout << "Already have a skybox cubmap, aborting." << std::endl;
    return;
  }
  else if (type == IRRADIANCE && this->irradiance != nullptr)
  {
    std::cout << "Already have a irradiance cubmap, aborting." << std::endl;
    return;
  }

  bool successful = true;
  unsigned char *data;

  switch (type)
  {
    case SKYBOX:
      this->skybox = new CubeMap();
      glGenTextures(1, &this->skybox->textureID);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textureID);

      for (unsigned i = 0; i < filenames.size(); i++)
      {
        data = stbi_load(filenames[i].c_str(), &this->skybox->width[i],
                         &this->skybox->height[i], &this->skybox->n[i], 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         this->skybox->width[i], this->skybox->height[i], 0,
                         GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: "
                      << filenames[i] << std::endl;
            stbi_image_free(data);
            successful = false;
            break;
        }
      }

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      break;
    case IRRADIANCE:
      this->irradiance = new CubeMap();
      glGenTextures(1, &this->irradiance->textureID);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradiance->textureID);

      for (unsigned i = 0; i < filenames.size(); i++)
      {
        data = stbi_load(filenames[i].c_str(), &this->irradiance->width[i],
                         &this->irradiance->height[i], &this->irradiance->n[i], 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         this->irradiance->width[i], this->irradiance->height[i], 0,
                         GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: "
                      << filenames[i] << std::endl;
            stbi_image_free(data);
            successful = false;
            break;
        }
      }

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      break;
    case PREFILTER:
      this->specPrefilter = new CubeMap();
      glGenTextures(1, &this->specPrefilter->textureID);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->specPrefilter->textureID);

      for (unsigned i = 0; i < filenames.size(); i++)
      {
        data = stbi_load(filenames[i].c_str(), &this->specPrefilter->width[i],
                         &this->specPrefilter->height[i], &this->specPrefilter->n[i], 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         this->specPrefilter->width[i], this->specPrefilter->height[i], 0,
                         GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: "
                      << filenames[i] << std::endl;
            stbi_image_free(data);
            successful = false;
            break;
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      }
      break;
    case INTEGRATION:
      this->brdfIntMap = new Texture2D();
      glGenTextures(1, &this->brdfIntMap->textureID);
      glBindTexture(GL_TEXTURE_2D, this->brdfIntMap->textureID);

      if (filenames.size() != 1)
      {
        std::cout << "Warning, multiple file paths provided. Only the first "
                  << "will be loaded as the BRDF lookup texture." << std::endl;
      }

      data = stbi_load(filenames[0].c_str(), &this->brdfIntMap->width,
                       &this->brdfIntMap->height, &this->brdfIntMap->n, 0);
      if (this->brdfIntMap->n != 2)
      {
        std::cout << "Invalid integration lookup texture (n = "
                  << this->brdfIntMap->n << ")." << std::endl;
        successful = false;
      }

      if (data)
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, this->brdfIntMap->width,
                     this->brdfIntMap->height, 0, GL_RG, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else
      {
        std::cout << "BRDF lookup texture failed to load at path: "
                  << filenames[0] << std::endl;
        successful = false;
      }

      break;
  }

  if (!successful)
  {
    switch (type)
    {
      case SKYBOX:
        glDeleteTextures(1, &this->skybox->textureID);
        delete this->skybox;
        this->skybox = nullptr;
        break;
      case IRRADIANCE:
        glDeleteTextures(1, &this->irradiance->textureID);
        delete this->irradiance;
        this->irradiance = nullptr;
        break;
      case PREFILTER:
        glDeleteTextures(1, &this->specPrefilter->textureID);
        delete this->specPrefilter;
        this->specPrefilter = nullptr;
        break;
      case INTEGRATION:
        glDeleteTextures(1, &this->brdfIntMap->textureID);
        delete this->brdfIntMap;
        this->brdfIntMap = nullptr;
        break;
    }
  }
}

// Bind/unbind a specific cubemap.
void
EnvironmentMap::bind(const MapType &type)
{
  switch (type)
  {
    case SKYBOX:
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textureID);
      break;
    case IRRADIANCE:
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradiance->textureID);
      break;
    case PREFILTER:
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->specPrefilter->textureID);
      break;
    case INTEGRATION:
      glBindTexture(GL_TEXTURE_2D, this->brdfIntMap->textureID);
      break;
  }
}

void
EnvironmentMap::unbind()
{
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void
EnvironmentMap::bindToPoint(const MapType &type, GLuint bindPoint)
{
  switch (type)
  {
    case SKYBOX:
      glActiveTexture(GL_TEXTURE0 + bindPoint);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textureID);
      break;
    case IRRADIANCE:
      glActiveTexture(GL_TEXTURE0 + bindPoint);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradiance->textureID);
      break;
    case PREFILTER:
      glActiveTexture(GL_TEXTURE0 + bindPoint);
      glBindTexture(GL_TEXTURE_CUBE_MAP, this->specPrefilter->textureID);
      break;
    case INTEGRATION:
      glActiveTexture(GL_TEXTURE0 + bindPoint);
      glBindTexture(GL_TEXTURE_2D, this->brdfIntMap->textureID);
      break;
  }
}

// Draw the skybox.
void
EnvironmentMap::draw(Camera* camera)
{
  Renderer* renderer = Renderer::getInstance();
  glDepthFunc(GL_LEQUAL);

  glm::mat4 vP = camera->getProjMatrix() * glm::mat4(glm::mat3(camera->getViewMatrix()));

  this->cubeShader->bind();
  this->cubeShader->addUniformMatrix("vP", vP, GL_FALSE);
  this->cubeShader->addUniformSampler2D("skybox", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textureID);

  if (this->cube->hasVAO())
    renderer->draw(this->cube->getVAO(), this->cubeShader);
  else
  {
    this->cube->generateVAO(this->cubeShader);
    if (this->cube->hasVAO())
      renderer->draw(this->cube->getVAO(), this->cubeShader);
  }

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  glDepthFunc(GL_LESS);
}

// Generate the diffuse irradiance map.
void
EnvironmentMap::precomputeIrradiance(GLuint width, GLuint height)
{
  Logger* logs = Logger::getInstance();

  if (this->irradiance != nullptr)
  {
    std::cout << "This environment already has an irradiance map. Aborting."
              << std::endl;
    return;
  }

  auto start = std::chrono::steady_clock::now();

  Renderer* renderer = Renderer::getInstance();

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
EnvironmentMap::precomputeSpecular(GLuint width, GLuint height)
{
  if (this->specPrefilter != nullptr && this->brdfIntMap != nullptr)
  {
    std::cout << "This environment already has the required specular maps. Aborting."
              << std::endl;
    return;
  }

  Logger* logs = Logger::getInstance();
  Renderer* renderer = Renderer::getInstance();

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

//------------------------------------------------------------------------------
// Misc. functions for texture manipulation.
//------------------------------------------------------------------------------

void
writeTexture(Texture2D* outTex)
{

}

void
writeTexture(CubeMap* outTex)
{

}
