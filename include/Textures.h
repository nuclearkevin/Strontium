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

    // Binds one of the environment map PBR textures to a point.
    void bindToPoint(const MapType &type, GLuint bindPoint);

    // Draw the skybox.
    void draw(Camera* camera);

    // Generate the diffuse irradiance map.
    void precomputeIrradiance(GLuint width, GLuint height);

    // Generate the specular map components (pre-filter and BRDF integration map).
    void precomputeSpecular(GLuint width, GLuint height, GLuint mipLevels);
  protected:
    CubeMap*   skybox;
    CubeMap*   irradiance;
    CubeMap*   specPrefilter;
    Texture2D* brdfIntMap;

    Shader*  cubeShader;

    Mesh*    cube;

    bool     hasSkybox;
    bool     hasIrradiance;
  };

  void writeTexture(Texture2D* outTex);
  void writeTexture(CubeMap* outTex);
}
