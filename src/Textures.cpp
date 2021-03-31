#define STB_IMAGE_IMPLEMENTATION

// Image loading include.
#include "stb_image/stb_image.h"

// Project includes.
#include "Textures.h"

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
  //std::cout << "Width: " << newTex->width << " Height: " << newTex->height <<
               //" N: " << newTex->n << std::endl;

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
  glActiveTexture(bindPoint);
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
  }

  if (!successful)
  {
    switch (type)
    {
      case SKYBOX:
        glDeleteTextures(1, &this->skybox->textureID);
        delete this->skybox;
        break;
      case IRRADIANCE:
        glDeleteTextures(1, &this->irradiance->textureID);
        delete this->irradiance;
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
  }
}

void
EnvironmentMap::unbind()
{
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
  glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradiance->textureID);

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

void
EnvironmentMap::precomputeIrradiance()
{
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
    // Resulting convolution is 25% the size of the original (since we don't
    // need a high resolution for diffuse reflection).
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                 512, 512, 0, GL_RGB,
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
                        512, 512);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, convoRBO);

  // The convolution program.
  Shader* convoProg = new Shader("./res/r_shaders/pbr/convolution.vs",
                                 "./res/r_shaders/pbr/convolution.fs");
  convoProg->addUniformSampler2D("environmentMap", 0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->textureID);

  glViewport(0, 0, 512, 512);

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
    irradiance->width[i]  = 512;
    irradiance->height[i] = 512;
    irradiance->n[i]      = 3;
  }

  // Delete the frame and render buffers now that they've served their purpose.
  glDeleteFramebuffers(1, &convoFBO);
  glDeleteRenderbuffers(1, &convoRBO);
}
