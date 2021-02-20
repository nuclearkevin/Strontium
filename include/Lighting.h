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
  enum LightType { UNIFORM, POINT, SPOT };

  struct LightMaterial
  {
    glm::vec3 diffuse;
    glm::vec3 specular;
    GLfloat   shininess;
  };

  struct UniformLight
  {
    glm::vec3 colour;
    glm::vec3 direction;

    LightMaterial mat;

    GLuint lightID;
  };

  struct PointLight
  {
    glm::vec3 colour;
    glm::vec4 position;

    GLfloat intensity;

    LightMaterial mat;

    GLuint lightID;
  };

  struct SpotLight
  {
    glm::vec3 colour;
    glm::vec4 position;
    glm::vec3 direction;

    GLfloat intensity;
    GLfloat innerCutOff;
    GLfloat outerCutOff;

    LightMaterial mat;

    GLuint lightID;
  };

  class LightController
  {
  public:
    LightController(const char* vertPath, const char* fragPath, const char* lightMeshPath);
    ~LightController() = default;

    // Add lights to this controller.
    void addLight(const UniformLight& light);
    void addLight(const PointLight& light, GLfloat scaleFactor);
    void addLight(const SpotLight& light, GLfloat scaleFactor);

    // Sets up the lights for rendering and draw the light meshes.
    void setLighting(Shader* lightingShader);
    void drawLightMeshes(Renderer* renderer, Camera* camera);

    // Remove a light from its lighting ID.
    void deleteLight(LightType type, GLuint lightID);

    // Getters.
    UniformLight* getULight(GLuint uLightID);
    PointLight*   getPLight(GLuint pLightID);
    SpotLight*    getSLightID(GLuint sLightID);
  protected:
    // Program specifically for rendering the light sources.
    Shader*                   lightProgram;

    // The mesh for the lights.
    Mesh*                     lightMesh;

    // The lights.
    std::vector<UniformLight> uniformLights;
    std::vector<PointLight>   pointLights;
    std::vector<SpotLight>    spotLights;
    std::vector<glm::mat4>    pointLightModels;
    std::vector<glm::mat4>    spotLightModels;
  };
}
