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
  //std::cout << "Width: " << newTex.width << " Height: " << newTex.height << std::endl;

  // Something went wrong while loading, abort.
  if (!data)
  {
    std::cout << "Failed to load image." << std::endl;
    return;
  }

  // Generate an RGB 2D texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, newTex->width, newTex->height, 0,
               GL_RGB, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

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
Texture2DController::unbindTexture()
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
{
  this->cubeShader = new Shader(vertPath, fragPath);
  this->cube = new Mesh();
  this->cube->loadOBJFile(cubeMeshPath);
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

      for (unsigned int i = 0; i < filenames.size(); i++)
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

      for (unsigned int i = 0; i < filenames.size(); i++)
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
EnvironmentMap::draw(Renderer* renderer, Camera* camera)
{
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

  glDepthFunc(GL_LESS);
}
