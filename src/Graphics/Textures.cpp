#include "Graphics/Textures.h"


// Project includes.
#include "Core/Logs.h"
#include "Core/Events.h"
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"

// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

namespace Strontium
{
  //----------------------------------------------------------------------------
  // 2D textures.
  //----------------------------------------------------------------------------
  Texture2D*
  Texture2D::createMonoColour(const glm::vec4 &colour, std::string &outName,
                              const Texture2DParams &params, bool cache)
  {
    Logger* logs = Logger::getInstance();
    auto textureCache = AssetManager<Texture2D>::getManager();

    Texture2D* outTex;
    if (cache)
    {
      if (!textureCache->hasAsset("Monocolour texture: " + Styles::colourToHex(colour)))
      {
        outTex = new Texture2D(1, 1, 4, params);
        textureCache->attachAsset("Monocolour texture: "
                                  + Styles::colourToHex(colour), outTex);

        logs->logMessage(LogMessage("Generated monocolour texture: " +
                                    Styles::colourToHex(colour) + ".",
                                    true, true));
      }
      else
      {
        outName = "Monocolour texture: " + Styles::colourToHex(colour);
        return textureCache->getAsset("Monocolour texture: "
                                      + Styles::colourToHex(colour));
      }
    }
    else
      outTex = new Texture2D(1, 1, 4, params);

    outName = "Monocolour texture: " + Styles::colourToHex(colour);

    outTex->bind();

    float* data = new float[4];
    data[0] = colour.r;
    data[1] = colour.g;
    data[2] = colour.b;
    data[3] = colour.a;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1, 1, 0,
                 GL_RGBA, GL_FLOAT, data);
    return outTex;
  }

  Texture2D*
  Texture2D::createMonoColour(const glm::vec4 &colour,
                              const Texture2DParams &params, bool cache)
  {
    Logger* logs = Logger::getInstance();
    auto textureCache = AssetManager<Texture2D>::getManager();

    Texture2D* outTex;
    if (cache)
    {
      if (!textureCache->hasAsset("Monocolour texture: " + Styles::colourToHex(colour)))
      {
        outTex = new Texture2D(1, 1, 4, params);
        textureCache->attachAsset("Monocolour texture: "
                                  + Styles::colourToHex(colour), outTex);

        logs->logMessage(LogMessage("Generated monocolour texture: " +
                                    Styles::colourToHex(colour) + ".",
                                    true, true));
      }
      else
      {
        return textureCache->getAsset("Monocolour texture: "
                                      + Styles::colourToHex(colour));
      }
    }
    else
      outTex = new Texture2D(1, 1, 4, params);

    outTex->bind();

    float* data = new float[4];
    data[0] = colour.r;
    data[1] = colour.g;
    data[2] = colour.b;
    data[3] = colour.a;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1, 1, 0,
                 GL_RGBA, GL_FLOAT, data);

    return outTex;
  }

  Texture2D*
  Texture2D::loadTexture2D(const std::string &filepath, const Texture2DParams &params,
                           bool cache)
  {
    Logger* logs = Logger::getInstance();
    auto textureCache = AssetManager<Texture2D>::getManager();

    bool isHDR = (filepath.substr(filepath.find_last_of("."), 4) == ".hdr");

    std::string name = filepath.substr(filepath.find_last_of('/') + 1);

    // The data.
    float* dataF;
    unsigned char* dataU;

    int width, height, n;

    // Load the texture.
    stbi_set_flip_vertically_on_load(true);
    if (isHDR)
      dataF = stbi_loadf(filepath.c_str(), &width, &height, &n, 0);
    else
      dataU = stbi_load(filepath.c_str(), &width, &height, &n, 0);

    // Something went wrong while loading, abort.
    if (!dataU && !isHDR)
    {
      logs->logMessage(LogMessage("Failed to load image at: " + filepath + ".",
                                  true, true));
      stbi_image_free(dataU);
    }
    else if (!dataF && isHDR)
    {
      logs->logMessage(LogMessage("Failed to load HDR image at: " + filepath + ".",
                                  true, true));
      stbi_image_free(dataF);
    }

    // The loaded texture.
    Texture2D* outTex;
    if (cache)
    {
      if (!textureCache->hasAsset(filepath))
      {
        outTex = new Texture2D(width, height, n, params);
        textureCache->attachAsset(name, outTex);
      }
      else
      {
        logs->logMessage(LogMessage("Fetched texture at: " + name + ".",
                                    true, true));
        return textureCache->getAsset(name);
      }
    }
    else
      outTex = new Texture2D(width, height, n, params);
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
      stbi_image_free(dataF);
    else
      stbi_image_free(dataU);

    logs->logMessage(LogMessage("Loaded texture at: " + filepath + ".",
                                true, true));

    outTex->getFilepath() = filepath;
    return outTex;
  }

  Texture2D::Texture2D()
    : filepath("")
  {
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
  }

  Texture2D::Texture2D(const GLuint &width, const GLuint &height, const GLuint &n,
                       const Texture2DParams &params)
    : width(width)
    , height(height)
    , n(n)
    , params(params)
    , filepath("")
  {
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_2D, this->textureID);

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
    glDeleteTextures(1, &this->textureID);
  }

  void
  Texture2D::bind()
  {
    glBindTexture(GL_TEXTURE_2D, this->textureID);
  }

  void
  Texture2D::bind(GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
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
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
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

    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);

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
    glDeleteTextures(1, &this->textureID);
  }

  void
  CubeMap::bind()
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
  }

  void
  CubeMap::bind(GLuint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
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
}
