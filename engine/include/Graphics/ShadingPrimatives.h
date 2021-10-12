#pragma once

namespace Strontium
{
  struct Camera
  {
    float near;
    float far;
    float fov;

    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 invViewProj;

    glm::vec3 position;
    glm::vec3 front;

    Camera()
      : near(0.1f)
      , far(30.0f)
      , fov(glm::radians(90.0f))
      , view(glm::mat4(1.0f))
      , projection(glm::mat4(1.0f))
      , invViewProj(glm::inverse(projection * view))
      , position(glm::vec3(0.0))
      , front(glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f)))
    { }
  };

  struct DirectionalLight
  {
    glm::vec3 direction;
    glm::vec3 colour;
    float intensity;
    bool castShadows;
    bool primaryLight;

    DirectionalLight()
      : direction(glm::vec3(0.0f, 1.0f, 0.0f))
      , colour(glm::vec3(1.0f))
      , intensity(0.0f)
      , castShadows(false)
      , primaryLight(false)
    { }
  };

  struct PointLight
  {
    glm::vec3 position;
    glm::vec3 colour;
    float intensity;
    float radius;
    float falloff;
    bool castShadows;

    PointLight()
      : position(glm::vec3(0.0f))
      , colour(glm::vec3(1.0f))
      , intensity(0.0f)
      , radius(0.0f)
      , falloff(0.0f)
      , castShadows(false)
    { }
  };

  struct SpotLight
  {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 colour;
    float intensity;
    float innerCutoff;
    float outerCutoff;
    float radius;
    bool castShadows;

    SpotLight()
      : position(glm::vec3(0.0f))
      , direction(glm::vec3(0.0f, 1.0f, 0.0f))
      , colour(glm::vec3(1.0f))
      , intensity(0.0f)
      , innerCutoff(std::cos(glm::radians(45.0f)))
      , outerCutoff(std::cos(glm::radians(90.0f)))
      , radius(0.0f)
      , castShadows(false)
    { }
  };
}
