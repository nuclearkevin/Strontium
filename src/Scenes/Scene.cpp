#include "Scenes/Scene.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Scenes/Components.h"
#include "Scenes/Entity.h"

namespace SciRenderer
{
  Scene::Scene()
  { }

  Scene::~Scene()
  { }

  Entity
  Scene::createEntity(const std::string& name)
  {
    Entity newEntity = Entity(this->sceneECS.create(), this);
    newEntity.addComponent<NameComponent>(name, "");
    return newEntity;
  }

  Entity
  Scene::createEntity(GLuint entityID, const std::string& name)
  {
    Entity newEntity = Entity(this->sceneECS.create(), this);
    newEntity.addComponent<NameComponent>(name, "");
    return newEntity;
  }

  void
  Scene::deleteEntity(Entity entity)
  {
    this->sceneECS.destroy(entity);
  }

  void
  Scene::onUpdate(float dt, Shared<Camera> sceneCamera)
  {
    auto modelAssets = AssetManager<Model>::getManager();

    // Group together the transform and renderable components.
    auto drawables = this->sceneECS.group<RenderableComponent>(entt::get<TransformComponent>);
    for (auto entity : drawables)
    {
      // Draw all the renderables with transforms.
      auto [transform, renderable] = drawables.get<TransformComponent, RenderableComponent>(entity);

      // Submit the mesh + material + transform to the deferred renderer queue.
      if (renderable)
      {
        auto& materials = renderable.materials.getStorage();
        auto& submeshes = modelAssets->getAsset(renderable.meshName)->getSubmeshes();

        Renderer3D::submit(renderable, renderable, transform);
      }
    }

    // Group together the lights and submit them to the renderer.
    auto dirLight = this->sceneECS.group<DirectionalLightComponent>(entt::get<TransformComponent>);
    for (auto entity : dirLight)
    {
      auto [directional, transform] = dirLight.get<DirectionalLightComponent, TransformComponent>(entity);
      Renderer3D::submit(directional, transform);
    }
    auto pointLight = this->sceneECS.group<PointLightComponent>(entt::get<TransformComponent>);
    for (auto entity : pointLight)
    {
      auto [point, transform] = pointLight.get<PointLightComponent, TransformComponent>(entity);
      Renderer3D::submit(point, transform);
    }
    auto spotLight = this->sceneECS.group<SpotLightComponent>(entt::get<TransformComponent>);
    for (auto entity : spotLight)
    {
      auto [spot, transform] = spotLight.get<SpotLightComponent, TransformComponent>(entity);
      Renderer3D::submit(spot, transform);
    }
  }
}
