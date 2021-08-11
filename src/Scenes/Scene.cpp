#include "Scenes/Scene.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Scenes/Components.h"
#include "Scenes/Entity.h"

namespace Strontium
{
  Scene::Scene(const std::string &filepath)
    : saveFilepath(filepath)
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
  Scene::recurseDeleteEntity(Entity entity)
  {
    if (entity.hasComponent<ParentEntityComponent>())
    {
      auto& parent = entity.getComponent<ParentEntityComponent>().parent;
      auto& parentChildren = parent.getComponent<ChildEntityComponent>().children;

      auto pos = std::find(parentChildren.begin(), parentChildren.end(), entity);
      if (pos != parentChildren.end())
        parentChildren.erase(pos);

      if (entity.hasComponent<ChildEntityComponent>())
      {
        auto& children = entity.getComponent<ChildEntityComponent>().children;
        for (auto& child : children)
          this->recurseDeleteEntity(child);
      }
    }
    else
    {
      if (entity.hasComponent<ChildEntityComponent>())
      {
        auto& children = entity.getComponent<ChildEntityComponent>().children;
        for (auto& child : children)
          this->recurseDeleteEntity(child);
      }
    }

    this->deleteEntity(entity);
  }

  void
  Scene::deleteEntity(Entity entity)
  {
    this->sceneECS.destroy(entity);
  }

  void
  Scene::onUpdate(float dt)
  {

  }

  void
  Scene::render(Entity selectedEntity)
  {
    // Group together the lights and submit them to the renderer.
    auto dirLight = this->sceneECS.group<DirectionalLightComponent>(entt::get<TransformComponent>);
    for (auto entity : dirLight)
    {
      auto [directional, transform] = dirLight.get<DirectionalLightComponent, TransformComponent>(entity);
      Renderer3D::submit(directional.light, transform);
    }
    auto pointLight = this->sceneECS.group<PointLightComponent>(entt::get<TransformComponent>);
    for (auto entity : pointLight)
    {
      auto [point, transform] = pointLight.get<PointLightComponent, TransformComponent>(entity);
      glm::mat4 transformMatrix = (glm::mat4) transform;

      // If a drawable item has a transform hierarchy, compute the global
      // transforms from local transforms.
      auto currentEntity = Entity(entity, this);
      if (currentEntity.hasComponent<ParentEntityComponent>())
        transformMatrix = computeGlobalTransform(currentEntity);

      Renderer3D::submit(point, transformMatrix);
    }
    auto spotLight = this->sceneECS.group<SpotLightComponent>(entt::get<TransformComponent>);
    for (auto entity : spotLight)
    {
      auto [spot, transform] = spotLight.get<SpotLightComponent, TransformComponent>(entity);
      glm::mat4 transformMatrix = (glm::mat4) transform;

      // If a drawable item has a transform hierarchy, compute the global
      // transforms from local transforms.
      auto currentEntity = Entity(entity, this);
      if (currentEntity.hasComponent<ParentEntityComponent>())
        transformMatrix = computeGlobalTransform(currentEntity);
        
      Renderer3D::submit(spot, transformMatrix);
    }

    // Group together the transform and renderable components.
    auto drawables = this->sceneECS.group<RenderableComponent>(entt::get<TransformComponent>);
    for (auto entity : drawables)
    {
      // Draw all the renderables with transforms.
      auto [transform, renderable] = drawables.get<TransformComponent, RenderableComponent>(entity);
      glm::mat4 transformMatrix = (glm::mat4) transform;

      // If a drawable item has a transform hierarchy, compute the global
      // transforms from local transforms.
      auto currentEntity = Entity(entity, this);
      if (currentEntity.hasComponent<ParentEntityComponent>())
        transformMatrix = computeGlobalTransform(currentEntity);

      bool selected = entity == selectedEntity;

      // Submit the mesh + material + transform to the deferred renderer queue.
      if (renderable)
        Renderer3D::submit(renderable, renderable, transformMatrix, static_cast<float>(entity), selected);
    }
  }

  // Compute the global transform given a parent-child transform hierarchy.
  // Only rotations and translation transforms.
  glm::mat4
  Scene::computeGlobalTransform(Entity entity)
  {
    if (entity.hasComponent<ParentEntityComponent>())
    {
      auto& parent = entity.getComponent<ParentEntityComponent>().parent;
      if (entity.hasComponent<TransformComponent>())
      {
        auto& localTransform = entity.getComponent<TransformComponent>();
        auto& localTranslation = localTransform.translation;
        auto scalelessTransform = glm::translate(glm::mat4(1.0f), localTranslation);
        return scalelessTransform * computeGlobalTransform(parent);
      }
      else
        return computeGlobalTransform(parent);
    }
    else
    {
      if (entity.hasComponent<TransformComponent>())
      {
        auto& localTransform = entity.getComponent<TransformComponent>();
        auto& localTranslation = localTransform.translation;
        auto scalelessTransform = glm::translate(glm::mat4(1.0f), localTranslation);
        return scalelessTransform;
      }
      else
        return glm::mat4(1.0f);
    }
  }
}
