// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// Project includes.
#include "Graphics/Textures.h"
#include "Core/Logs.h"

namespace SciRenderer
{
  //----------------------------------------------------------------------------
  // 2D textures.
  //----------------------------------------------------------------------------
  Texture2D::Texture2D()
  {
    glGenTextures(1, &this->getID());
    glBindTexture(GL_TEXTURE_2D, this->getID());
  }

  Texture2D::Texture2D(const GLuint &width, const GLuint &height, const GLuint &n,
                       const Texture2DParams &params)
    : width(width)
    , height(height)
    , n(n)
    , params(params)
  {
    glGenTextures(1, &this->getID());
    glBindTexture(GL_TEXTURE_2D, this->getID());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    static_cast<GLint>(params.sWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    static_cast<GLint>(params.tWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    static_cast<GLint>(params.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    static_cast<GLint>(params.maxFilter));
  }

  Texture2D::~Texture2D()
  {
    glDeleteTextures(1, &this->getID());
  }

  void
  Texture2D::bind()
  {
    glBindTexture(GL_TEXTURE_2D, this->getID());
  }

  void
  Texture2D::bind(GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_2D, this->getID());
  }

  void
  Texture2D::unbind()
  {
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void
  Texture2D::unbind(GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  //----------------------------------------------------------------------------
  // Cubemap textures.
  //----------------------------------------------------------------------------
  CubeMap::CubeMap()
  {
    glGenTextures(1, &this->getID());
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->getID());
  }

  CubeMap::CubeMap(const GLuint &width, const GLuint &height, const GLuint &n,
                   const TextureCubeMapParams &params)
    : params(params)
  {
    for (GLuint i = 0; i < 6; i++)
    {
      this->width[i] = width;
      this->height[i] = height;
      this->n[i] = n;
    }

    glGenTextures(1, &this->getID());
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->getID());

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                    static_cast<GLint>(params.sWrap));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                    static_cast<GLint>(params.tWrap));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                    static_cast<GLint>(params.rWrap));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    static_cast<GLint>(params.minFilter));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                    static_cast<GLint>(params.maxFilter));
  }

  CubeMap::~CubeMap()
  {
    glDeleteTextures(1, &this->getID());
  }

  void
  CubeMap::bind()
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->getID());
  }

  void
  CubeMap::bind(GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->getID());
  }

  void
  CubeMap::unbind()
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  void
  CubeMap::unbind(GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  //----------------------------------------------------------------------------
  // Misc. functions for texture manipulation.
  //----------------------------------------------------------------------------
  Texture2D*
  Textures::loadTexture2D(const std::string &filepath,
                          const Texture2DParams &params, bool isHDR)
  {
    Logger* logs = Logger::getInstance();

    // The data.
    float* dataF;
    unsigned char* dataU;

    int width, height, n;

    // Load the texture.
    stbi_set_flip_vertically_on_load(true);
    if (isHDR)
      dataF = stbi_loadf(filepath.c_str(), &width, &height, &n, 0);
    else
      dataU = stbi_load(filepath.c_str(), &width, &height, &n, 0);;

    // Something went wrong while loading, abort.
    if (!dataU && !isHDR)
    {
      logs->logMessage(LogMessage("Failed to load image at: " + filepath + ".",
                                  true, false, true));
      stbi_image_free(dataU);
    }
    else if (!dataF && isHDR)
    {
      logs->logMessage(LogMessage("Failed to load HDR image at: " + filepath + ".",
                                  true, false, true));
      stbi_image_free(dataF);
    }

    // The loaded texture.
    Texture2D* outTex = new Texture2D(width, height, n, params);
    outTex->bind();

    // Generate a 2D texture. Currently supports both bytes and floating point
    // HDR images!
    if (outTex->n == 1)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0,
                     GL_RED, GL_FLOAT, dataF);
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0,
                     GL_RED, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 2)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0,
                     GL_RG, GL_FLOAT, dataF);
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width, height, 0,
                     GL_RG, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 3)
    {
      // If its HDR, needs to be GL_RGBA16F instead of GL_RGB16F. Thanks OpenGL....
      if (isHDR)
      {
        float* dataFNew;
        dataFNew = new float[width * height * 4];
        GLuint offset = 0;

        for (GLuint i = 0; i < (width * height * 4); i+=4)
        {
          // Copy over the data from the image loading.
          dataFNew[i] = dataF[i - offset];
          dataFNew[i + 1] = dataF[i + 1 - offset];
          dataFNew[i + 2] = dataF[i + 2 - offset];
          // Make the 4th component (alpha) equal to 1.0f. Could make this a param :thinking:.
          dataFNew[i + 3] = 1.0f;
          // Increment the offset to we don't segfault. :D
          offset ++;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                     GL_RGBA, GL_FLOAT, dataFNew);
        stbi_image_free(dataFNew);
      }
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 4)
    {
      if (isHDR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                     GL_RGBA, GL_FLOAT, dataF);
      else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, dataU);
      glGenerateMipmap(GL_TEXTURE_2D);
    }

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

    if (successful)
    {
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, static_cast<GLint>(params.sWrap));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, static_cast<GLint>(params.tWrap));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, static_cast<GLint>(params.rWrap));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(params.minFilter));
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(params.maxFilter));
      outMap->params = params;

      logs->logMessage(LogMessage("Cubemap texture successfully loaded.",
                                  true, false, false));

      return outMap;
    }
  }

  // Write textures to disk.
  // TODO: Finish these. Non-critical for functionality though.
  void
  Textures::writeTexture2D(Shared<Texture2D> &outTex)
  {

  }

  void
  Textures::writeTextureCubeMap(Shared<CubeMap> &outTex)
  {

  }
}
