#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Graphics/Meshes.h"
#include "Graphics/Material.h"
#include "Graphics/EnvironmentMap.h"
#include "Graphics/Renderer.h"

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

    TransformComponent(const TransformComponent&) = default;

    TransformComponent()
      : translation(glm::vec3(0.0f))
      , rotation(glm::vec3(0.0f))
      , scale(glm::vec3(1.0f))
    { }

    TransformComponent(const glm::vec3 &translation, const glm::vec3 &rotation,
                       const glm::vec3 &scale)
      : translation(translation)
      , rotation(rotation)
      , scale(scale)
    { }

    // Cast to glm::mat4 for easy multiplication.
    operator glm::mat4()
    {
      return glm::translate(glm::mat4(1.0f), translation)
             * glm::toMat4(glm::quat(rotation)) * glm::scale(scale);
    }
  };

  // Renderable component. This is the component which is passed to the renderer
  // alongside the transform component.
  struct RenderableComponent
  {
    // A model and a collection of materials for the model's submeshes.
    ModelMaterial materials;

    // Name so we can fetch the associated model.
    std::string meshName;

    RenderableComponent(const RenderableComponent&) = default;

    RenderableComponent()
      : meshName("")
    { }

    RenderableComponent(const std::string &meshName)
      : meshName(meshName)
    {
      auto modelAssets = AssetManager<Model>::getManager();

      Model* model = modelAssets->getAsset(meshName);

      if (model != nullptr)
      {
        for (auto& pair : model->getSubmeshes())
        {
          materials.attachMesh(pair.second->getName());
        }
      }
    }

    operator Model*()
    {
      auto modelAssets = AssetManager<Model>::getManager();

      return modelAssets->getAsset(meshName);
    }

    operator ModelMaterial&() { return materials; }

    operator bool()
    {
      auto modelAssets = AssetManager<Model>::getManager();

      return modelAssets->getAsset(meshName) != nullptr;
    }
  };

  // This is an IBL ambient light component. TODO: Finish and overhaul environment maps.
  struct AmbientComponent
  {
    EnvironmentMap* ambient;

    AmbientComponent(const AmbientComponent&) = default;

    AmbientComponent()
    {
      ambient = Renderer3D::getStorage()->currentEnvironment.get();
    }

    AmbientComponent(const std::string &iblImagePath)
    {
      ambient = Renderer3D::getStorage()->currentEnvironment.get();
      auto state = Renderer3D::getState();

      ambient->loadEquirectangularMap(iblImagePath);
      ambient->equiToCubeMap(true, state->skyboxWidth, state->skyboxWidth);
      ambient->precomputeIrradiance(state->irradianceWidth, state->irradianceWidth, true);
      ambient->precomputeSpecular(state->prefilterWidth, state->prefilterWidth, true);
    }
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
    GLfloat radius;

    bool castShadows;

    PointLightComponent(const PointLightComponent&) = default;

    PointLightComponent()
      : position(glm::vec3(0.0f))
      , colour(glm::vec3(1.0f))
      , intensity(0.0f)
      , radius(0.0f)
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
    GLfloat radius;

    bool castShadows;

    SpotLightComponent(const SpotLightComponent&) = default;

    SpotLightComponent()
      : position(glm::vec3(0.0f))
      , direction(glm::vec3(0.0f, -1.0f, 0.0f))
      , colour(glm::vec4(1.0f))
      , intensity(0.0f)
      , innerCutoff(std::cos(glm::radians(45.0f)))
      , outerCutoff(std::cos(glm::radians(90.0f)))
      , radius(0.0f)
      , castShadows(false)
    { }
  };
}
