#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// Project includes.
#include "Graphics/Textures.h"
#include "Core/Logs.h"

namespace SciRenderer
{
  //----------------------------------------------------------------------------
  // 2D texture controller here.
  //----------------------------------------------------------------------------
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
  Texture2DController::loadFomFile(const char* filepath,
                                   const std::string &texName)
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

  //----------------------------------------------------------------------------
  // Misc. functions for texture manipulation.
  //----------------------------------------------------------------------------

  void
  writeTexture(Texture2D* outTex)
  {

  }

  void
  writeTexture(CubeMap* outTex)
  {

  }
}
