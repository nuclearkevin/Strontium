#include "Graphics/Textures.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/Events.h"
#include "Assets/AssetManager.h"
#include "Utils/Utilities.h"

// OpenGL includes.
#include "glad/glad.h"

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
      if (!textureCache->hasAsset("Monocolour texture: " + Utilities::colourToHex(colour)))
      {
        outTex = new Texture2D(1, 1, 4, params);
        textureCache->attachAsset("Monocolour texture: "
                                  + Utilities::colourToHex(colour), outTex);

        logs->logMessage(LogMessage("Generated monocolour texture: " +
                                    Utilities::colourToHex(colour) + ".",
                                    true, true));
      }
      else
      {
        outName = "Monocolour texture: " + Utilities::colourToHex(colour);
        return textureCache->getAsset("Monocolour texture: "
                                      + Utilities::colourToHex(colour));
      }
    }
    else
      outTex = new Texture2D(1, 1, 4, params);

    outName = "Monocolour texture: " + Utilities::colourToHex(colour);

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
      if (!textureCache->hasAsset("Monocolour texture: " + Utilities::colourToHex(colour)))
      {
        outTex = new Texture2D(1, 1, 4, params);
        textureCache->attachAsset("Monocolour texture: "
                                  + Utilities::colourToHex(colour), outTex);

        logs->logMessage(LogMessage("Generated monocolour texture: " +
                                    Utilities::colourToHex(colour) + ".",
                                    true, true));
      }
      else
      {
        return textureCache->getAsset("Monocolour texture: "
                                      + Utilities::colourToHex(colour));
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
    float* dataF = nullptr;
    unsigned char* dataU = nullptr;

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
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0,
                     GL_RED, GL_FLOAT, dataF);
        outTex->params.dataType = TextureDataType::Floats;
        outTex->params.format = TextureFormats::Red;
        outTex->params.internal = TextureInternalFormats::R16f;
      }
      else
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0,
                     GL_RED, GL_UNSIGNED_BYTE, dataU);
        outTex->params.format = TextureFormats::Red;
        outTex->params.internal = TextureInternalFormats::Red;
      }
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 2)
    {
      if (isHDR)
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0,
                     GL_RG, GL_FLOAT, dataF);
        outTex->params.dataType = TextureDataType::Floats;
        outTex->params.format = TextureFormats::RG;
        outTex->params.internal = TextureInternalFormats::RG16f;
      }
      else
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width, height, 0,
                     GL_RG, GL_UNSIGNED_BYTE, dataU);
        outTex->params.format = TextureFormats::RG;
        outTex->params.internal = TextureInternalFormats::RG;
      }
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 3)
    {
      // If its HDR, needs to be GL_RGBA16F instead of GL_RGB16F. Thanks OpenGL....
      if (isHDR)
      {
        float* dataFNew = new float[width * height * 4];
        uint offset = 0;

        for (uint i = 0; i < (width * height * 4); i+=4)
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
        outTex->params.dataType = TextureDataType::Floats;
        outTex->params.format = TextureFormats::RGBA;
        outTex->params.internal = TextureInternalFormats::RGBA16f;
        stbi_image_free(dataFNew);
      }
      else
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, dataU);
        outTex->params.format = TextureFormats::RGB;
        outTex->params.internal = TextureInternalFormats::RGB;
      }
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (outTex->n == 4)
    {
      if (isHDR)
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                     GL_RGBA, GL_FLOAT, dataF);
        outTex->params.dataType = TextureDataType::Floats;
        outTex->params.format = TextureFormats::RGBA;
        outTex->params.internal = TextureInternalFormats::RGBA16f;
      }
      else
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, dataU);
        outTex->params.format = TextureFormats::RGBA;
        outTex->params.internal = TextureInternalFormats::RGBA;
      }
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

  Texture2DParams 
  Texture2D::getDefaultColourParams()
  {
    Texture2DParams defaultColour = Texture2DParams();
    defaultColour.internal = TextureInternalFormats::RGB;
    defaultColour.format = TextureFormats::RGB;
    defaultColour.dataType = TextureDataType::Bytes;
    defaultColour.sWrap = TextureWrapParams::Repeat;
    defaultColour.tWrap = TextureWrapParams::Repeat;
    defaultColour.minFilter = TextureMinFilterParams::Linear;
    defaultColour.maxFilter = TextureMaxFilterParams::Linear;
    return defaultColour;
  }

  Texture2DParams 
  Texture2D::getFloatColourParams()
  {
    Texture2DParams floatColour = Texture2DParams();
    floatColour.internal = TextureInternalFormats::RGBA16f;
    floatColour.format = TextureFormats::RGBA;
    floatColour.dataType = TextureDataType::Floats;
    floatColour.sWrap = TextureWrapParams::Repeat;
    floatColour.tWrap = TextureWrapParams::Repeat;
    floatColour.minFilter = TextureMinFilterParams::Linear;
    floatColour.maxFilter = TextureMaxFilterParams::Linear;
    return floatColour;
  }

  Texture2DParams 
  Texture2D::getDefaultDepthParams()
  {
    Texture2DParams defaultDepth = Texture2DParams();
    defaultDepth.internal = TextureInternalFormats::Depth32f;
    defaultDepth.format = TextureFormats::Depth;
    defaultDepth.dataType = TextureDataType::Floats;
    defaultDepth.sWrap = TextureWrapParams::Repeat;
    defaultDepth.tWrap = TextureWrapParams::Repeat;
    defaultDepth.minFilter = TextureMinFilterParams::Nearest;
    defaultDepth.maxFilter = TextureMaxFilterParams::Nearest;
    return defaultDepth;
  }

  Texture2D::Texture2D()
    : filepath("")
    , width(0)
    , height(0)
    , n(0)
  {
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
  }

  Texture2D::Texture2D(const uint &width, const uint &height, const uint &n,
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
                    static_cast<GLenum>(this->params.sWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    static_cast<GLenum>(this->params.tWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    static_cast<GLenum>(this->params.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    static_cast<GLenum>(this->params.maxFilter));
  }

  Texture2D::~Texture2D()
  {
    glDeleteTextures(1, &this->textureID);
  }

  // Init the texture using given data.
  void
  Texture2D::initNullTexture()
  {
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(this->params.internal),
                 this->width, this->height, 0, static_cast<GLenum>(this->params.format),
                 static_cast<GLenum>(this->params.dataType), nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void
  Texture2D::loadData(const float* data)
  {
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(this->params.internal),
                 this->width, this->height, 0, static_cast<GLenum>(this->params.format),
                 static_cast<GLenum>(this->params.dataType), data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void
  Texture2D::loadData(const unsigned char* data)
  {
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(this->params.internal),
                 this->width, this->height, 0, static_cast<GLenum>(this->params.format),
                 static_cast<GLenum>(this->params.dataType), data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // Generate mipmaps.
  void
  Texture2D::generateMips()
  {
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // Clear the texture.
  /*
  Depth = GL_DEPTH_COMPONENT, DepthStencil = GL_DEPTH_STENCIL,
  Depth24Stencil8 = GL_DEPTH24_STENCIL8, Depth32f = GL_DEPTH_COMPONENT32F,
  Red = GL_RED, RG = GL_RG, RGB = GL_RGB, RGBA = GL_RGBA, R16f = GL_R16F,
  RG16f = GL_RG16F, RGB16f = GL_RGB16F, RGBA16f = GL_RGBA16F, R32f = GL_R32F,
  RG32f = GL_RG32F, RGB32f = GL_RGB32F, RGBA32f = GL_RGBA32F
  */
  void
  Texture2D::clearTexture()
  {
    switch (this->n)
    {
      case 1:
      {
        float clearElement = 0.0f;
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width, this->height, 0,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement);
        break;
      }
      case 2:
      {
        glm::vec2 clearElement = glm::vec2(0.0f);
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width, this->height, 0,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement.x);
        break;
      }
      case 3:
      {
        glm::vec3 clearElement = glm::vec3(0.0f);
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width, this->height, 0,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement.x);
        break;
      }
      case 4:
      {
        glm::vec4 clearElement = glm::vec4(0.0f);
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width, this->height, 0,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement.x);
        break;
      }
      default:
      {
        return;
      }
    }
  }

  void
  Texture2D::setSize(uint width, uint height, uint n)
  {
    this->width = width;
    this->height = height;
    this->n = n;
  }

  // Set the parameters after generating the texture.
  void
  Texture2D::setParams(const Texture2DParams &newParams)
  {
    this->params = newParams;

    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    static_cast<GLint>(this->params.sWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    static_cast<GLint>(this->params.tWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    static_cast<GLint>(this->params.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    static_cast<GLint>(this->params.maxFilter));
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void
  Texture2D::bind()
  {
    glBindTexture(GL_TEXTURE_2D, this->textureID);
  }

  void
  Texture2D::bind(uint bindPoint)
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
  Texture2D::unbind(uint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // Bind the texture as an image unit.
  void
  Texture2D::bindAsImage(uint bindPoint, uint miplevel, ImageAccessPolicy policy)
  {
    glBindImageTexture(bindPoint, this->textureID, miplevel, GL_FALSE, 0,
                       static_cast<GLenum>(policy),
                       static_cast<GLenum>(this->params.internal));
  }

  Texture2DArray::Texture2DArray()
    : width(0)
    , height(0)
    , n(0)
    , numLayers(0)
  {
    glGenTextures(1, &this->textureID);
  }

  Texture2DArray::Texture2DArray(uint width, uint height, uint n, 
                                 uint numLayers, const Texture2DParams& params)
    : width(width)
    , height(height)
    , n(n)
    , numLayers(numLayers)
    , params(params)
  {
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
        static_cast<GLint>(this->params.sWrap));
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
        static_cast<GLint>(this->params.tWrap));
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
        static_cast<GLint>(this->params.minFilter));
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
        static_cast<GLint>(this->params.maxFilter));
  }

  Texture2DArray::~Texture2DArray()
  {
    glDeleteTextures(1, &this->textureID);
  }

  // Init the texture using given data and stored params.
  void 
  Texture2DArray::initNullTexture()
  {
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureID);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 0, static_cast<GLenum>(this->params.internal), 
                   this->width, this->height, this->numLayers);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  }

  // Set the parameters after generating the texture.
  void 
  Texture2DArray::setSize(uint width, uint height, uint n, uint numLayers)
  {
    this->width = width;
    this->height = height;
    this->n = n;
    this->numLayers = numLayers;
  }

  void 
  Texture2DArray::setParams(const Texture2DParams& newParams)
  {
    this->params = newParams;

    glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureID);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                    static_cast<GLint>(this->params.sWrap));
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                    static_cast<GLint>(this->params.tWrap));
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                    static_cast<GLint>(this->params.minFilter));
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
                    static_cast<GLint>(this->params.maxFilter));
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  }

  // Generate mipmaps. TODO
  void 
  Texture2DArray::generateMips()
  {

  }

  // Clear the texture. TODO
  void 
  Texture2DArray::clearTexture()
  {

  }

  // Bind/unbind the texture.
  void 
  Texture2DArray::bind()
  {
    glBindTexture(GL_TEXTURE_2D, this->textureID);
  }

  void 
  Texture2DArray::bind(uint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
  }

  void 
  Texture2DArray::unbind()
  {
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void 
  Texture2DArray::unbind(uint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // Bind the texture as an image unit.
  void 
  Texture2DArray::bindAsImage(uint bindPoint, uint miplevel, bool isLayered,
                              uint layer, ImageAccessPolicy policy)
  {
    glBindImageTexture(bindPoint, this->textureID, miplevel,
                       static_cast<GLenum>(isLayered), layer,
                       static_cast<GLenum>(policy),
                       static_cast<GLenum>(this->params.internal));
  }

  //----------------------------------------------------------------------------
  // Cubemap textures.
  //----------------------------------------------------------------------------
  CubeMap::CubeMap()
  {
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);

    for (uint i = 0; i < 6; i++)
    {
      this->width[i] = 0;
      this->height[i] = 0;
      this->n[i] = 0;
    }
  }

  CubeMap::CubeMap(const uint &width, const uint &height, const uint &n,
                   const TextureCubeMapParams &params)
    : params(params)
  {
    for (uint i = 0; i < 6; i++)
    {
      this->width[i] = width;
      this->height[i] = height;
      this->n[i] = n;
    }

    glGenTextures(1, &this->textureID);

    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                    static_cast<GLenum>(params.sWrap));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                    static_cast<GLenum>(params.tWrap));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                    static_cast<GLenum>(params.rWrap));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    static_cast<GLenum>(params.minFilter));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                    static_cast<GLenum>(params.maxFilter));
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  CubeMap::~CubeMap()
  {
    glDeleteTextures(1, &this->textureID);
  }

  void
  CubeMap::initNullTexture()
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
    for (unsigned int i = 0; i < 6; i++)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, static_cast<GLenum>(this->params.internal),
                  this->width[i], this->height[i], 0, static_cast<GLenum>(this->params.format),
                  static_cast<GLenum>(this->params.dataType), nullptr);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  void
  CubeMap::generateMips()
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  // Clear the texture.
  void
  CubeMap::clearTexture()
  {
    switch (this->n[0])
    {
      case 1:
      {
        float clearElement = 0.0f;
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width[0], this->height[0], 6,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement);
        break;
      }
      case 2:
      {
        glm::vec2 clearElement = glm::vec2(0.0f);
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width[0], this->height[0], 6,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement.x);
        break;
      }
      case 3:
      {
        glm::vec3 clearElement = glm::vec3(0.0f);
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width[0], this->height[0], 6,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement.x);
        break;
      }
      case 4:
      {
        glm::vec4 clearElement = glm::vec4(0.0f);
        glClearTexSubImage(this->textureID, 0, 0, 0, 0, this->width[0], this->height[0], 6,
                           static_cast<GLenum>(this->params.internal),
                           static_cast<GLenum>(this->params.dataType),
                           &clearElement.x);
        break;
      }
      default:
      {
        return;
      }
    }
  }

  void
  CubeMap::setSize(uint width, uint height, uint n)
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      this->width[i] = width;
      this->height[i] = height;
      this->n[i] = n;
    }
  }

  // Set the parameters after generating the texture.
  void
  CubeMap::setParams(const TextureCubeMapParams &newParams)
  {
    this->params = newParams;

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
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  void
  CubeMap::bind()
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
  }

  void
  CubeMap::bind(uint bindPoint)
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
  CubeMap::unbind(uint bindPoint)
  {
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  // Bind the texture as an image unit.
  void
  CubeMap::bindAsImage(uint bindPoint, uint miplevel, bool isLayered,
                       uint layer, ImageAccessPolicy policy)
  {
    glBindImageTexture(bindPoint, this->textureID, miplevel,
                       static_cast<GLenum>(isLayered), layer,
                       static_cast<GLenum>(policy),
                       static_cast<GLenum>(this->params.internal));
  }
}
