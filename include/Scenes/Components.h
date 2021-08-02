#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Graphics/Meshes.h"
#include "Graphics/Material.h"
#include "Graphics/EnvironmentMap.h"
#include "Graphics/Renderer.h"
#include "Scenes/Entity.h"

namespace Strontium
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

  //----------------------------------------------------------------------------
  // Internal components. Cannot be added or removed by a user.
  //----------------------------------------------------------------------------
  // A parent entity component so each child entity knows its parents.
  struct ParentEntityComponent
  {
    Entity parent;

    ParentEntityComponent(const ParentEntityComponent&) = default;

    ParentEntityComponent(Entity parent)
      : parent(parent)
    { }

    ParentEntityComponent() = default;
  };

  // A child entity component so each parent component knows its children.
  struct ChildEntityComponent
  {
    std::vector<Entity> children;

    ChildEntityComponent(const ChildEntityComponent&) = default;

    ChildEntityComponent() = default;
  };

  // Prefab component so each prefab can be iterated over and updated in synch.
  struct PrefabComponent
  {
    std::string prefabID;
    std::string prefabPath;
    bool synch;

    PrefabComponent(const PrefabComponent&) = default;

    PrefabComponent(const std::string &prefabID, const std::string &prefabPath)
      : prefabID(prefabID)
      , synch(false)
      , prefabPath(prefabPath)
    { }
  };
  //----------------------------------------------------------------------------

  // Entity transform component.
  struct TransformComponent
  {
    glm::vec3 translation;
    glm::vec3 rotation; // Euler angles. Pitch = x, yaw = y, roll = z.
    glm::vec3 scale;

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
    { }

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
    Renderer3D::DirectionalLight light;

    DirectionalLightComponent(const DirectionalLightComponent&) = default;

    DirectionalLightComponent()
      : light()
    { }

    operator Renderer3D::DirectionalLight()
    {
      return light;
    }
  };

  struct PointLightComponent
  {
    Renderer3D::PointLight light;

    PointLightComponent(const PointLightComponent&) = default;

    PointLightComponent()
      : light()
    { }

    operator Renderer3D::PointLight()
    {
      return light;
    }
  };

  struct SpotLightComponent
  {
    Renderer3D::SpotLight light;

    SpotLightComponent(const SpotLightComponent&) = default;

    SpotLightComponent()
      : light()
    { }

    operator Renderer3D::SpotLight()
    {
      return light;
    }
  };
}
