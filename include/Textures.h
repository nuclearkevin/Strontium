#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Meshes.h"
#include "Shaders.h"
#include "Renderer.h"
#include "Camera.h"

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

  enum MapType { SKYBOX, IRRADIANCE };

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
    void unbindTexture();

    // Binds a texture to a point.
    void bindToPoint(const std::string &texName, GLuint bindPoint);

    // Get a texture ID.
    GLuint getTexID(const std::string &texName);
  protected:
    // Store the textures in a map for fast access.
    std::unordered_map<std::string, Texture2D*> textures;
    std::vector<std::string>                    texNames;
  };

  class EnvironmentMap
  {
  public:
    EnvironmentMap(const char* vertPath, const char* fragPath,
                   const char* cubeMeshPath);
    ~EnvironmentMap();

    // Load 6 textures from a file to generate a cubemap.
    void loadCubeMap(const std::vector<std::string> &filenames,
                     const MapType &type);

    // Bind/unbind a specific cubemap.
    void bind(const MapType &type);
    void unbind();

    // Draw the skybox.
    void draw(Renderer* renderer, Camera* camera);
  protected:
    CubeMap* skybox;
    CubeMap* irradiance;

    Shader*  cubeShader;

    Mesh*    cube;

    bool     hasSkybox;
    bool     hasIrradiance;
  };
}
