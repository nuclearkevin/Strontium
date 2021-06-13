#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/Meshes.h"
#include "Graphics/Material.h"
#include "Graphics/EnvironmentMap.h"

namespace SciRenderer
{
  // Entity nametag component.
  struct NameComponent
  {
    std::string name;
    std::string description;

    NameComponent(const NameComponent&) = default;

    NameComponent(NameComponent& other)
    {
      name = other.name + " (Copy)";
      description = other.description;
    }

    NameComponent()
      : name("")
      , description("")
    { }

    NameComponent(const std::string &name, const std::string &description)
      : name(name)
      , description(description)
    { }

    friend std::ostream& operator<<(std::ostream& os, const NameComponent &component)
    {
      os << "Name: " << component.name << "\n" << "Description: "
         << component.description;
      return os;
    }

    operator std::string()
    {
      return "Name: " + name + "\n" + "Description: " + description;
    }
  };

  // Entity transform component.
  struct TransformComponent
  {
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale; // Euler angles. Pitch = x, yaw = y, roll = z.
    GLfloat scaleFactor;

    TransformComponent(const TransformComponent&) = default;

    TransformComponent()
      : translation(glm::vec3(0.0f))
      , rotation(glm::vec3(0.0f))
      , scale(glm::vec3(1.0f))
      , scaleFactor(1.0f)
    { }

    TransformComponent(const glm::vec3 &translation, const glm::vec3 &rotation,
                       const glm::vec3 &scale)
      : translation(translation)
      , rotation(rotation)
      , scale(scale)
      , scaleFactor(1.0f)
    { }

    // Cast to glm::mat4 for easy multiplication.
    operator glm::mat4()
    {
      return glm::translate(glm::mat4(1.0f), translation)
             * glm::toMat4(glm::quat(rotation)) * glm::scale(scale * scaleFactor);
    }
  };

  // Renderable component. This is the component which is passed to the renderer
  // alongside the transform component.
  struct RenderableComponent
  {
    // A model and a collection of materials for the model's submeshes.
    Model* model;
    ModelMaterial materials;

    // Names so we can fetch the assets easily.
    std::string meshName;

    RenderableComponent(const RenderableComponent&) = default;

    RenderableComponent()
      : meshName("")
    {
      model = new Model();
    }

    RenderableComponent(Model* model, const std::string &meshName)
      : model(model)
      , meshName(meshName)
    {
      for (auto& pair : model->getSubmeshes())
      {
        materials.attachMesh(pair.second);
      }
    }

    operator Model*() { return model; }
    operator ModelMaterial&() { return materials; }
    operator bool() { return model != nullptr; }
  };

  // This is an IBL ambient light component. TODO: Finish and overhaul environment maps.
  struct AmbientComponent
  {
    Shared<EnvironmentMap> ambient;

    // Environment map properties.
    GLfloat roughness;
    GLfloat gamma;
    GLfloat ambientFactor;

    bool drawingMips;

    AmbientComponent(const AmbientComponent&) = default;

    AmbientComponent()
      : roughness(0.0f)
      , gamma(2.2f)
      , ambientFactor(1.0f)
      , drawingMips(false)
    {
      ambient = createShared<EnvironmentMap>("./res/models/cube.obj");
    }

    AmbientComponent(Shared<EnvironmentMap> map)
      : ambient(map)
      , roughness(0.0f)
      , gamma(2.2f)
      , ambientFactor(1.0f)
      , drawingMips(false)
    { }

    AmbientComponent(const std::string &iblImagePath)
      : roughness(0.0f)
      , gamma(2.2f)
      , ambientFactor(1.0f)
      , drawingMips(false)
    {
      ambient = createShared<EnvironmentMap>("./res/models/cube.obj");
      ambient->loadEquirectangularMap(iblImagePath);
      ambient->equiToCubeMap(true, 2048, 2048);
      ambient->precomputeIrradiance(512, 512, true);
      ambient->precomputeSpecular(2048, 2048, true);
    }

    operator bool() { return ambient != nullptr; }
    operator Shared<EnvironmentMap>() { return ambient; }
  };

  // TODO: Finish these.
  // Various light components for rendering the scene.
  struct DirectionalLightComponent
  {
    glm::vec3 direction;
    glm::vec3 colour;

    GLfloat intensity;

    bool castShadows;

    DirectionalLightComponent(const DirectionalLightComponent&) = default;

    DirectionalLightComponent()
      : direction(glm::vec3(0.0f, -1.0f, 0.0f))
      , colour(glm::vec3(1.0f))
      , intensity(0.0f)
      , castShadows(false)
    { }
  };

  struct PointLightComponent
  {
    glm::vec3 position;
    glm::vec3 colour;

    GLfloat intensity;
    glm::vec2 attenuation;

    bool castShadows;

    PointLightComponent(const PointLightComponent&) = default;

    PointLightComponent()
      : position(glm::vec3(0.0f))
      , colour(glm::vec3(1.0f))
      , intensity(0.0f)
      , attenuation(glm::vec2(0.0f))
      , castShadows(false)
    { }
  };

  struct SpotLightComponent
  {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec4 colour;

    GLfloat intensity;
    GLfloat innerCutoff;
    GLfloat outerCutoff;
    glm::vec2 attenuation;

    bool castShadows;

    SpotLightComponent(const SpotLightComponent&) = default;

    SpotLightComponent()
      : position(glm::vec3(0.0f))
      , direction(glm::vec3(0.0f, -1.0f, 0.0f))
      , colour(glm::vec4(1.0f))
      , intensity(0.0f)
      , innerCutoff(std::cos(glm::radians(45.0f)))
      , outerCutoff(std::cos(glm::radians(90.0f)))
      , attenuation(glm::vec2(0.0f))
      , castShadows(false)
    { }
  };
}
