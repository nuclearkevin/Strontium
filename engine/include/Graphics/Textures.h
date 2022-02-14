#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

namespace Strontium
{
  // Parameters for textures.
  enum class TextureInternalFormats
  {
    Depth = 0x1902, // GL_DEPTH_COMPONENT
    DepthStencil = 0x84F9, // GL_DEPTH_STENCIL
    Depth24Stencil8 = 0x88F0, // GL_DEPTH24_STENCIL8
    Depth32f = 0x8CAC, // GL_DEPTH_COMPONENT32F
    Red = 0x1903, // GL_RED
    RG = 0x8227, // GL_RG
    RGB = 0x1907, // GL_RGB
    RGBA = 0x1908, // GL_RGBA
    R16f = 0x822D, // GL_R16F
    RG16f = 0x822F, // GL_RG16F
    RGB16f = 0x881B, // GL_RGB16F
    RGBA16f = 0x881A,  // GL_RGBA16F
    R32f = 0x822E, // GL_R32F
    RG32f = 0x8230, // GL_RG32F
    RGB32f = 0x8815, // GL_RGB32F
    RGBA32f = 0x8814 // GL_RGBA32F
  };
  enum class TextureFormats
  {
    Depth = 0x1902, // GL_DEPTH_COMPONENT
    DepthStencil = 0x84F9, // GL_DEPTH_STENCIL
    Red = 0x1903, // GL_RED
    RG = 0x8227, // GL_RG
    RGB = 0x1907, // GL_RGB
    RGBA = 0x1908 // GL_RGBA
  };
  enum class TextureDataType
  {
    Bytes = 0x1401, // GL_UNSIGNED_BYTE
    Floats = 0x1406, // GL_FLOAT
    UInt24UInt8 = 0x84FA // GL_UNSIGNED_INT_24_8
  };
  enum class TextureWrapParams
  {
    ClampEdges = 0x812F, // GL_CLAMP_TO_EDGE
    ClampBorder = 0x812D, // GL_CLAMP_TO_BORDER
    MirroredRepeat = 0x8370, // GL_MIRRORED_REPEAT
    Repeat = 0x2901, // GL_REPEAT
    MirrorClampEdges = 0x8743 // GL_MIRROR_CLAMP_TO_EDGE
  };
  enum class TextureMinFilterParams
  {
    Nearest = 0x2600, // GL_NEAREST
    Linear = 0x2601, // GL_LINEAR
    NearestMipMapNearest = 0x2700, // GL_NEAREST_MIPMAP_NEAREST
    LinearMipMapNearest = 0x2701, // GL_LINEAR_MIPMAP_NEAREST
    LinearMipMapLinear = 0x2703 // GL_LINEAR_MIPMAP_LINEAR
  };
  enum class TextureMaxFilterParams
  {
    Nearest = 0x2600, // GL_NEAREST
    Linear = 0x2601 // GL_LINEAR
  };
  enum class CubemapFace
  {
    PosX = 0x8515, // GL_TEXTURE_CUBE_MAP_POSITIVE_X
    NegX = 0x8516, // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    PosY = 0x8517, // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    NegY = 0x8518, // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    PosZ = 0x8519, // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    NegZ = 0x851A // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
  };
  enum class ImageAccessPolicy
  {
    Read = 0x88B8, // GL_READ_ONLY
    Write = 0x88B9, // GL_WRITE_ONLY
    ReadWrite = 0x88BA // GL_READ_WRITE
  };

  // Texture parameters.
  struct Texture2DParams
  {
    TextureWrapParams      sWrap;
    TextureWrapParams      tWrap;
    TextureMinFilterParams minFilter;
    TextureMaxFilterParams maxFilter;

    TextureInternalFormats internal;
    TextureFormats         format;
    TextureDataType        dataType;

    Texture2DParams()
      : sWrap(TextureWrapParams::Repeat)
      , tWrap(TextureWrapParams::Repeat)
      , minFilter(TextureMinFilterParams::Linear)
      , maxFilter(TextureMaxFilterParams::Linear)
      , internal(TextureInternalFormats::RGBA)
      , format(TextureFormats::RGBA)
      , dataType(TextureDataType::Bytes)
    { };
  };

  struct TextureCubeMapParams
  {
    TextureWrapParams      sWrap;
    TextureWrapParams      tWrap;
    TextureWrapParams      rWrap;
    TextureMinFilterParams minFilter;
    TextureMaxFilterParams maxFilter;

    TextureInternalFormats internal;
    TextureFormats         format;
    TextureDataType        dataType;

    TextureCubeMapParams()
      : sWrap(TextureWrapParams::ClampEdges)
      , tWrap(TextureWrapParams::ClampEdges)
      , rWrap(TextureWrapParams::ClampEdges)
      , minFilter(TextureMinFilterParams::Linear)
      , maxFilter(TextureMaxFilterParams::Linear)
      , internal(TextureInternalFormats::RGBA)
      , format(TextureFormats::RGBA)
      , dataType(TextureDataType::Bytes)
    { };
  };

  // Texture parameters.
  struct Texture3DParams
  {
    TextureWrapParams      sWrap;
    TextureWrapParams      tWrap;
    TextureWrapParams      rWrap;
    TextureMinFilterParams minFilter;
    TextureMaxFilterParams maxFilter;

    TextureInternalFormats internal;
    TextureFormats         format;
    TextureDataType        dataType;

    Texture3DParams()
      : sWrap(TextureWrapParams::Repeat)
      , tWrap(TextureWrapParams::Repeat)
      , rWrap(TextureWrapParams::Repeat)
      , minFilter(TextureMinFilterParams::Linear)
      , maxFilter(TextureMaxFilterParams::Linear)
      , internal(TextureInternalFormats::RGBA)
      , format(TextureFormats::RGBA)
      , dataType(TextureDataType::Bytes)
    { };
  };

  struct ImageData2D
  {
    int width;
    int height;
    int n;

    void* data;

    bool isHDR;
    Texture2DParams params;
    std::string name;
    std::string filepath;
  };

  //----------------------------------------------------------------------------
  // 2D textures.
  //----------------------------------------------------------------------------
  class Texture2D
  {
  public:
    // Other members to load and generate textures.
    static Texture2D* createMonoColour(const glm::vec4 &colour, std::string &outName,
                                       const Texture2DParams &params = Texture2DParams(),
                                       bool cache = true);
    static Texture2D* createMonoColour(const glm::vec4 &colour, const Texture2DParams &params = Texture2DParams(),
                                       bool cache = true);

    // This loads an image and generates the texture all at once, does so on the
    // main thread due to OpenGL thread safety.
    static Texture2D* loadTexture2D(const std::string &filepath, const Texture2DParams &params
                                    = Texture2DParams(), bool cache = true);

    static Texture2DParams getDefaultColourParams();
    static Texture2DParams getFloatColourParams();
    static Texture2DParams getDefaultDepthParams();

    // The actual 2D texture class.
    Texture2D();
    Texture2D(uint width, uint height, const Texture2DParams &params = Texture2DParams());
    ~Texture2D();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // Init the texture using given data and stored params.
    void initNullTexture();
    void loadData(const float* data);
    void loadData(const unsigned char* data);

    // Set the parameters after generating the texture.
    void setSize(uint width, uint height);
    void setParams(const Texture2DParams &newParams);

    // Generate mipmaps.
    void generateMips();

    // Clear the texture.
    void clearTexture();

    // Bind/unbind the texture.
    void bind();
    void bind(uint bindPoint);
    void unbind();
    void unbind(uint bindPoint);

    // Bind the texture as an image unit.
    void bindAsImage(uint bindPoint, uint miplevel, ImageAccessPolicy policy);

    int getWidth() { return this->width; }
    int getHeight() { return this->height; }

    uint& getID() { return this->textureID; }
    std::string& getFilepath() { return this->filepath; }
  private:
    uint textureID;

    int width;
    int height;
    Texture2DParams params;

    std::string filepath;
  };

  class Texture2DArray
  {
  public:
    Texture2DArray();
    Texture2DArray(uint width, uint height, uint numLayers,
                   const Texture2DParams &params = Texture2DParams());
    ~Texture2DArray();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    Texture2DArray(const Texture2DArray&) = delete;
    Texture2DArray& operator=(const Texture2DArray&) = delete;

    // Init the texture using given data and stored params.
    void initNullTexture();

    // Set the parameters after generating the texture.
    void setSize(uint width, uint height, uint numLayers);
    void setParams(const Texture2DParams& newParams);

    // Generate mipmaps.
    void generateMips();

    // Clear the texture.
    void clearTexture();

    // Bind/unbind the texture.
    void bind();
    void bind(uint bindPoint);
    void unbind();
    void unbind(uint bindPoint);

    // Bind the texture as an image unit.
    void bindAsImage(uint bindPoint, uint miplevel, bool isLayered,
                     uint layer, ImageAccessPolicy policy);

    int getWidth() { return this->width; }
    int getHeight() { return this->height; }
    int getLayers() { return this->numLayers; }

    uint& getID() { return this->textureID; }
  private:
    uint textureID;

    uint width;
    uint height;
    uint numLayers;
    Texture2DParams params;
  };

  //----------------------------------------------------------------------------
  // Cubemap textures.
  //----------------------------------------------------------------------------
  class CubeMap
  {
  public:
    CubeMap();
    CubeMap(uint width, uint height, const TextureCubeMapParams &params = TextureCubeMapParams());
    ~CubeMap();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    CubeMap(const CubeMap&) = delete;
    CubeMap& operator=(const CubeMap&) = delete;

    // Init the texture using given data and stored params.
    void initNullTexture();

    // Generate mipmaps.
    void generateMips();

    // Clear the texture.
    void clearTexture();

    // Set the parameters after generating the texture.
    void setSize(uint width, uint height);
    void setParams(const TextureCubeMapParams &newParams);

    // Bind/unbind the texture.
    void bind();
    void bind(uint bindPoint);
    void unbind();
    void unbind(uint bindPoint);

    // Bind the texture as an image unit.
    void bindAsImage(uint bindPoint, uint miplevel, bool isLayered,
                     uint layer, ImageAccessPolicy policy);

    int getWidth(uint face) { return this->width[face]; }
    int getHeight(uint face) { return this->height[face]; }

    uint& getID() { return this->textureID; }
    std::string& getFilepath() { return this->filepath; }
  private:
    uint textureID;

    int width[6];
    int height[6];
    TextureCubeMapParams params;

    std::string filepath;
  };

  class CubeMapArrayTexture
  {
  public:
    CubeMapArrayTexture();
    CubeMapArrayTexture(uint width, uint height, uint numLayers,
                        const TextureCubeMapParams &params = TextureCubeMapParams());
    ~CubeMapArrayTexture();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    CubeMapArrayTexture(const CubeMapArrayTexture&) = delete;
    CubeMapArrayTexture& operator=(const CubeMapArrayTexture&) = delete;

    // Init the texture using given data and stored params.
    void initNullTexture();

    // Generate mipmaps.
    void generateMips();

    // Clear the texture.
    void clearTexture();

    // Set the parameters after generating the texture.
    void setSize(uint width, uint height, uint numLayers);
    void setParams(const TextureCubeMapParams& newParams);

    // Bind/unbind the texture.
    void bind();
    void bind(uint bindPoint);
    void unbind();
    void unbind(uint bindPoint);

    // Bind the texture as an image unit.
    void bindAsImage(uint bindPoint, uint miplevel, bool isLayered,
                     uint layer, ImageAccessPolicy policy);

    int getWidth(uint face) { return this->width[face]; }
    int getHeight(uint face) { return this->height[face]; }

    uint& getID() { return this->textureID; }
  private:
    uint textureID;

    int width[6];
    int height[6];
    int numLayers;
    TextureCubeMapParams params;
  };

  class Texture3D
  {
  public:
    Texture3D();
    Texture3D(uint width, uint height, uint depth, 
              const Texture3DParams &params = Texture3DParams());
    ~Texture3D();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    Texture3D(const Texture3D&) = delete;
    Texture3D& operator=(const Texture3D&) = delete;
    
    // Init the texture using given data and stored params.
    void initNullTexture();

    // Generate mipmaps.
    void generateMips();

    // Clear the texture.
    void clearTexture();

    // Set the parameters after generating the texture.
    void setSize(uint width, uint height, uint depth);
    void setParams(const Texture3DParams& newParams);

    // Bind/unbind the texture.
    void bind();
    void bind(uint bindPoint);
    void unbind();
    void unbind(uint bindPoint);

    // Bind the texture as an image unit.
    void bindAsImage(uint bindPoint, uint miplevel, bool isLayered,
                     uint layer, ImageAccessPolicy policy);

    uint& getID() { return this->textureID; }
  private:
    uint textureID;

    int width;
    int height;
    int depth;

    Texture3DParams params;
  };
}
