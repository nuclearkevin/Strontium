#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/Shaders.h"
#include "Graphics/Renderer.h"
#include "Graphics/Meshes.h"
#include "Graphics/Camera.h"

namespace SciRenderer
{
  enum LightType { UNIFORM, POINT, SPOT, ALL };

  // Structs to store light information.
  struct LightMaterial
  {
    // 12 float components.
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec2 attenuation;
    GLfloat   shininess;

    LightMaterial()
      : diffuse(glm::vec4(0.0f))
      , specular(glm::vec4(0.0f))
      , attenuation(glm::vec2(0.0f))
      , shininess(1.0f)
    { }
  };

  struct UniformLight
  {
    glm::vec3 colour;
    glm::vec3 direction;

    GLfloat intensity;

    LightMaterial mat;

    std::string   name;
    GLuint        lightID;

    UniformLight()
      : colour(glm::vec3(0.0f))
      , direction(glm::vec3(0.0f))
      , intensity(0.0f)
      , mat(LightMaterial())
    { }
  };

  struct PointLight
  {
    glm::vec3 colour;
    glm::vec3 position;

    GLfloat intensity;
    GLfloat meshScale;

    LightMaterial mat;

    std::string   name;
    GLuint        lightID;

    PointLight()
      : colour(glm::vec3(0.0f))
      , position(glm::vec3(0.0f))
      , intensity(0.0f)
      , meshScale(0.1f)
      , mat(LightMaterial())
    { }
  };

  struct SpotLight
  {
    glm::vec3 colour;
    glm::vec3 position;
    glm::vec3 direction;

    GLfloat intensity;
    GLfloat innerCutOff;
    GLfloat outerCutOff;
    GLfloat meshScale;

    LightMaterial mat;

    std::string   name;
    GLuint        lightID;

    SpotLight()
      : colour(glm::vec3(0.0f))
      , position(glm::vec3(0.0f))
      , intensity(0.0f)
      , innerCutOff(0.0f)
      , outerCutOff(0.0f)
      , meshScale(0.1f)
      , mat(LightMaterial())
    { }
  };

  // Structs which match their shader counterparts in lighting.fs
  struct ShaderUL
  {
    // 24 float components.
    glm::vec4     colour;
    glm::vec4     direction;
    glm::vec4     diffuse;
    glm::vec4     specular;
    glm::vec2     attenuation;
    GLfloat       shininess;
    GLfloat       intensity;
  };

  struct ShaderPL
  {
    // 24 float components.
    glm::vec4     colour;
    glm::vec4     position;
    glm::vec4     diffuse;
    glm::vec4     specular;
    glm::vec2     attenuation;
    GLfloat       shininess;
    GLfloat       intensity;
  };

  struct ShaderSL
  {
    // 28 float components.
    glm::vec4     colour;
    glm::vec4     position;
    glm::vec4     direction;
    glm::vec4     diffuse;
    glm::vec4     specular;
    glm::vec2     attenuation;
    GLfloat       shininess;
    GLfloat       intensity;
    GLfloat       cosTheta;
    GLfloat       cosGamma;
    GLfloat       padding[2];
  };

  class LightController
  {
  public:
    LightController(const std::string &vertPath, const std::string &fragPath,
                    const char* lightMeshPath);
    ~LightController();

    // Add lights to this controller.
    void addLight(const UniformLight& light);
    void addLight(const PointLight& light, GLfloat scaleFactor);
    void addLight(const SpotLight& light, GLfloat scaleFactor);

    // Sets up the lights for rendering and draw the light meshes.
    void setLighting(Shader* lightingShader, Camera* camera);
    void drawLightMeshes(Camera* camera);

    // Remove a light from its lighting ID.
    void deleteLight(LightType type, GLuint lightID);

    // Setting the gui labels.
    void setGuiLabel(LightType type);

    // Getters.
    glm::vec3*    getAmbient();
    UniformLight* getULight(GLuint uLightID);
    PointLight*   getPLight(GLuint pLightID);
    SpotLight*    getSLight(GLuint sLightID);
    GLuint        getNumLights(LightType type);

    std::vector<std::string> &getGuiLabel(LightType type);
  protected:
    // Program specifically for rendering the light source meshes.
    Shader*                   lightProgram;

    // Uniform buffers for the different light types.
    UniformBuffer*            ulightBuffer;
    UniformBuffer*            plightBuffer;
    UniformBuffer*            slightBuffer;

    // The mesh for the lights.
    Mesh*                     lightMesh;

    // The lights.
    glm::vec3                 ambient;
    std::vector<UniformLight> uniformLights;
    std::vector<PointLight>   pointLights;
    std::vector<SpotLight>    spotLights;

    std::vector<std::string>  uLGuiLabel;
    std::vector<std::string>  pLGuiLabel;
    std::vector<std::string>  sLGuiLabel;
    std::vector<std::string>  allLightGuiLabel;

    GLuint                    uLightCounter;
    GLuint                    pLightCounter;
    GLuint                    sLightCounter;
  };
}
