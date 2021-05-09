// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// Project includes.
#include "Graphics/Textures.h"
#include "Core/Logs.h"

namespace SciRenderer
{
  //----------------------------------------------------------------------------
  // 2D texture controller.
  //----------------------------------------------------------------------------
  Texture2DController::~Texture2DController()
  {
    // Frees all the textures from GPU memory when deleted.
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
  Texture2DController::loadFomFile(const std::string &filepath,
                                   const std::string &texName,
                                   Texture2DParams params,
                                   bool isHDR)
  {
    Texture2D* newTex = Textures::loadTexture2D(filepath, params, isHDR);

    // Push the texture to an unordered map.
    this->textures.insert({ texName, newTex });
    this->texNames.push_back(texName);
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
  Texture2D*
  Textures::loadTexture2D(const std::string &filepath,
                          Texture2DParams params, bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    // Struct for the texture information.
    Texture2D* outTex = new Texture2D();

    // The data.
    float* dataF;
    unsigned char* dataU;

    // Generate and bind the GPU texture.
    glGenTextures(1, &outTex->textureID);
    glBindTexture(GL_TEXTURE_2D, outTex->textureID);

    // Load the texture.
    stbi_set_flip_vertically_on_load(true);
    if (isHDR)
      dataF = stbi_loadf(filepath.c_str(), &outTex->width,
                         &outTex->height, &outTex->n, 0);
    else
      dataU = stbi_load(filepath.c_str(), &outTex->width,
                        &outTex->height, &outTex->n, 0);

    // Something went wrong while loading, abort.
    if (!dataU && !isHDR)
    {
      logs->logMessage(LogMessage("Failed to load image at: " + filepath + ".",
                                  true, false, true));
      stbi_image_free(dataU);
      glDeleteTextures(1, &outTex->textureID);
      delete outTex;
      return nullptr;
    }
    else if (!dataF && isHDR)
    {
      logs->logMessage(LogMessage("Failed to load HDR image at: " + filepath + ".",
                                  true, false, true));
      stbi_image_free(dataF);
      glDeleteTextures(1, &outTex->textureID);
      delete outTex;
      return nullptr;
    }

    // Generate a 2D texture. Currently supports both bytes and floating point
    // HDR images!
    if (outTex->n == 1)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, outTex->width, outTex->height, 0,
                     GL_RED, GL_FLOAT, dataF);
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, outTex->width, outTex->height, 0,
                     GL_RED, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 2)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, outTex->width, outTex->height, 0,
                     GL_RG, GL_FLOAT, dataF);
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, outTex->width, outTex->height, 0,
                     GL_RG, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 3)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, outTex->width, outTex->height, 0,
                     GL_RGB, GL_FLOAT, dataF);
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, outTex->width, outTex->height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 4)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, outTex->width, outTex->height, 0,
                     GL_RGBA, GL_FLOAT, dataF);
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outTex->width, outTex->height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(params.sWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(params.tWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(params.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(params.maxFilter));

    // Free memory.
    if (isHDR)
    {
      stbi_image_free(dataF);
      logs->logMessage(LogMessage("HDR 2D Texture successfully loaded.",
                                  true, false, false));
    }
    else
    {
      stbi_image_free(dataU);
      logs->logMessage(LogMessage("Non-HDR 2D Texture successfully loaded.",
                                  true, false, false));
    }

    return outTex;
  }

  CubeMap*
  Textures::loadTextureCubeMap(const std::vector<std::string> &filenames,
                               const TextureCubeMapParams &params,
                               const bool &isHDR)
  {
    Logger* logs = Logger::getInstance();

    // If there aren't enough faces, don't load.
    if (filenames.size() != 6)
    {
      logs->logMessage(LogMessage("Not enough faces to load a full cubemap.",
                                  true, false, true));
      return nullptr;
    }

    bool successful = true;
    unsigned char *data;

    CubeMap* outMap = new CubeMap();
    glGenTextures(1, &outMap->textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, outMap->textureID);

    stbi_set_flip_vertically_on_load(false);

    for (unsigned i = 0; i < filenames.size(); i++)
    {
      data = stbi_load(filenames[i].c_str(), &outMap->width[i],
                       &outMap->height[i], &outMap->n[i], 0);
      if (data)
      {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                     outMap->width[i], outMap->height[i], 0,
                     GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
      }
      else
      {
        logs->logMessage(LogMessage(std::string("Cubemap face failed to"
                                    " load at path: ") + filenames[i], true,
                                    false, true));
        stbi_image_free(data);
        successful = false;
        break;
      }
    }

    if (!successful)
    {
      glDeleteTextures(1, &outMap->textureID);
      delete outMap;
      return nullptr;
    }
    else
    {
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, static_cast<GLint>(params.sWrap));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, static_cast<GLint>(params.tWrap));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, static_cast<GLint>(params.rWrap));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(params.minFilter));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(params.maxFilter));

      logs->logMessage(LogMessage("Cubemap texture successfully loaded.",
                                  true, false, false));

      return outMap;
    }
  }

  // Write textures to disk.
  // TODO: Finish these. Non-critical for functionality though.
  void
  Textures::writeTexture2D(Texture2D* outTex)
  {

  }

  void
  Textures::writeTextureCubeMap(CubeMap* outTex)
  {

  }

  // Delete a texture and set the raw pointer to nullptr;
  void
  Textures::deleteTexture(Texture2D* tex)
  {
    glDeleteTextures(1, &tex->textureID);
    delete tex;
    tex = nullptr;
  }

  void
  Textures::deleteTexture(CubeMap* tex)
  {
    glDeleteTextures(1, &tex->textureID);
    delete tex;
    tex = nullptr;
  }

  // Bind a texture.
  void
  Textures::bindTexture(Texture2D* tex)
  {
    glBindTexture(GL_TEXTURE_2D, tex->textureID);
  }

  void
  Textures::bindTexture(CubeMap* tex)
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex->textureID);
  }

  // Unbind a texture.
  void
  Textures::unbindTexture2D()
  {
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void Textures::unbindCubeMap()
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  // Bind a texture to a specific binding point.
  void
  Textures::bindTexture(Texture2D* tex, GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_2D, tex->textureID);
  }

  void
  Textures::bindTexture(CubeMap* tex, GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex->textureID);
  }
}
