#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Graphics/Meshes.h"
#include "Graphics/Shaders.h"
#include "Graphics/Renderer.h"
#include "Graphics/Camera.h"
#include "Graphics/Buffers.h"

// STL includes.
#include <unordered_map>

namespace SciRenderer
{
  // Struct to store texture information.
  struct Texture2D
  {
    GLuint textureID;
    int width;
    int height;
    int n;
  };

  enum MapType { SKYBOX, IRRADIANCE, PREFILTER, INTEGRATION };

  // Struct to store a cube map.
  struct CubeMap
  {
    GLuint textureID;
    int width[6];
    int height[6];
    int n[6];
  };

  class Texture2DController
  {
  public:
    Texture2DController() = default;
    ~Texture2DController();

    // Load a texture from a file into the texture handler.
    void loadFomFile(const char* filepath, const std::string &texName);

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

  void writeTexture(Texture2D* outTex);
  void writeTexture(CubeMap* outTex);
}
