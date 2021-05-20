#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// STL includes.
#include <unordered_map>

namespace SciRenderer
{
  // Parameters for textures.
  enum class TextureInternalFormats
  {
    Depth = GL_DEPTH_COMPONENT, DepthStencil = GL_DEPTH_STENCIL,
    Depth24Stencil8 = GL_DEPTH24_STENCIL8, Red = GL_RED, RG = GL_RG,
    RGB = GL_RGB, RGBA = GL_RGBA, R16F = GL_R16F, RG16f = GL_RG16F,
    RGB16f = GL_RGB16F, RGBA16f = GL_RGBA16F
  };
  enum class TextureFormats
  {
    Depth = GL_DEPTH_COMPONENT, DepthStencil = GL_DEPTH_STENCIL, Red = GL_RED,
    RG = GL_RG, RGB = GL_RGB, RGBA = GL_RGBA
  };
  enum class TextureDataType
  {
    Bytes = GL_UNSIGNED_BYTE, Floats = GL_FLOAT,
    UInt24UInt8 = GL_UNSIGNED_INT_24_8
  };
  enum class TextureWrapParams
  {
    ClampEdges = GL_CLAMP_TO_EDGE, ClampBorder = GL_CLAMP_TO_BORDER,
    MirroredRepeat =  GL_MIRRORED_REPEAT, Repeat = GL_REPEAT,
    MirrorClampEdges =  GL_MIRROR_CLAMP_TO_EDGE
  };
  enum class TextureMinFilterParams
  {
    Nearest = GL_NEAREST, Linear = GL_LINEAR,
    NearestMipMapNearest = GL_NEAREST_MIPMAP_NEAREST,
    LinearMipMapNearest = GL_LINEAR_MIPMAP_NEAREST,
    LinearMipMapLinear = GL_LINEAR_MIPMAP_LINEAR
  };
  enum class TextureMaxFilterParams
  {
    Nearest = GL_NEAREST, Linear = GL_LINEAR
  };

  // Parameters for loading textures.
  struct Texture2DParams
  {
    TextureWrapParams      sWrap;
    TextureWrapParams      tWrap;
    TextureMinFilterParams minFilter;
    TextureMaxFilterParams maxFilter;

    Texture2DParams()
      : sWrap(TextureWrapParams::Repeat)
      , tWrap(TextureWrapParams::Repeat)
      , minFilter(TextureMinFilterParams::Linear)
      , maxFilter(TextureMaxFilterParams::Linear)
    { };
  };

  struct TextureCubeMapParams
  {
    TextureWrapParams      sWrap;
    TextureWrapParams      tWrap;
    TextureWrapParams      rWrap;
    TextureMinFilterParams minFilter;
    TextureMaxFilterParams maxFilter;

    TextureCubeMapParams()
      : sWrap(TextureWrapParams::ClampEdges)
      , tWrap(TextureWrapParams::ClampEdges)
      , rWrap(TextureWrapParams::ClampEdges)
      , minFilter(TextureMinFilterParams::Linear)
      , maxFilter(TextureMaxFilterParams::Linear)
    { };
  };

  // Struct to store texture information.
  struct Texture2D
  {
    GLuint textureID;
    int width;
    int height;
    int n;

    ~Texture2D()
    {

    };
  };

  // Struct to store a cube map.
  struct CubeMap
  {
    GLuint textureID;
    int width[6];
    int height[6];
    int n[6];

    ~CubeMap()
    {

    };
  };

  //----------------------------------------------------------------------------
  // 2D texture controller.
  //----------------------------------------------------------------------------
  class Texture2DController
  {
  public:
    Texture2DController() = default;
    ~Texture2DController();

    // Load a texture from a file into the texture handler.
    void loadFomFile(const std::string &filepath, const std::string &texName,
                     Texture2DParams params = Texture2DParams(),
                     bool isHDR = false);

    // Deletes a texture given its name.
    void deleteTexture(const std::string &texName);

    // Bind/unbind a texture.
    void bind(const std::string &texName);
    void unbind();

    // Binds a texture to a point.
    void bindToPoint(const std::string &texName, GLuint bindPoint);

    // Get a texture ID.
    GLuint getTexID(const std::string &texName);
  protected:
    // Store the textures in a map for fast access.
    std::unordered_map<std::string, Texture2D*> textures;
    std::vector<std::string>                    texNames;
  };

  //----------------------------------------------------------------------------
  // Misc. functions for texture manipulation. Wrapped in the Textures
  // namespace.
  //----------------------------------------------------------------------------
  namespace Textures
  {
    // Read textures from disk.
    Texture2D* loadTexture2D(const std::string &filepath,
                             Texture2DParams params = Texture2DParams(),
                             bool isHDR = false);
    CubeMap* loadTextureCubeMap(const std::vector<std::string> &filenames,
                                const TextureCubeMapParams &params = TextureCubeMapParams(),
                                const bool &isHDR = false);

    // Write textures to disk.
    void writeTexture2D(Texture2D* outTex);
    void writeTextureCubeMap(CubeMap* outTex);

    // Delete a texture and set the raw pointer to nullptr;
    void deleteTexture(Texture2D* &tex);
    void deleteTexture(CubeMap* &tex);

    // Bind a texture.
    void bindTexture(Texture2D* tex);
    void bindTexture(CubeMap* tex);

    // Bind a texture to a specific binding point.
    void bindTexture(Texture2D* tex, GLuint bindPoint);
    void bindTexture(CubeMap* tex, GLuint bindPoint);

    // Unbind a texture.
    void unbindTexture2D();
    void unbindCubeMap();
  }
}
