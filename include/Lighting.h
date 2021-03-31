#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Shaders.h"
#include "Renderer.h"
#include "Meshes.h"
#include "Camera.h"

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
  };

  struct UniformLight
  {
    glm::vec3 colour;
    glm::vec3 direction;

    GLfloat intensity;

    LightMaterial mat;

    std::string   name;
    GLuint        lightID;
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
    LightController(const char* vertPath, const char* fragPath,
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
