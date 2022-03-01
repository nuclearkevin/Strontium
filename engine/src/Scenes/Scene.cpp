#include "Scenes/Scene.h"

// Project includes.
#include "Scenes/Components.h"
#include "Scenes/Entity.h"

#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/RenderPassManager.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"
#include "Graphics/RenderPasses/IBLApplicationPass.h"
#include "Graphics/RenderPasses/SkyboxPass.h"

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
  Scene::createEntity(uint entityID, const std::string& name)
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
  Scene::onUpdateEditor(float dt)
  {
    this->updateAnimations(dt);
  }

  void
  Scene::onUpdateRuntime(float dt)
  {
    this->updateAnimations(dt);
  }

  void
  Scene::onRenderEditor(Entity selectedEntity)
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

      // Submit the mesh + material + transform to the static deferred renderer queue.
      if (renderable && !renderable.animator.animationRenderable())
        Renderer3D::submit(renderable, renderable, transformMatrix,
                           static_cast<float>(entity), selected);
      // If it has a valid animation, instead submit it to the dynamic deferred renderer queue.
      else if (renderable && renderable.animator.animationRenderable())
        Renderer3D::submit(renderable, &renderable.animator, renderable,
                           transformMatrix, static_cast<float>(entity), selected);
    }

    // Group together the transform, sky-atmosphere and directional light components.
    auto& passManager = Renderer3D::getPassManager();
    auto skyAtm = passManager.getRenderPass<SkyAtmospherePass>();
    auto dynIBL = passManager.getRenderPass<DynamicSkyIBLPass>();
    auto iblApp = passManager.getRenderPass<IBLApplicationPass>();
    auto skyboxApp = passManager.getRenderPass<SkyboxPass>();
    auto atmospheres = this->sceneECS.group<SkyAtmosphereComponent>(entt::get<TransformComponent>);
    for (auto entity : atmospheres)
    {
      auto [transform, atmosphere] = atmospheres.get<TransformComponent, SkyAtmosphereComponent>(entity);

      bool canComputeIBL = false;
      bool skyUpdated = false;
      if (this->sceneECS.has<DirectionalLightComponent>(entity) && !atmosphere.usePrimaryLight)
      {
        canComputeIBL = true;
        skyUpdated = skyAtm->submit(atmosphere, atmosphere, this->sceneECS.get<DirectionalLightComponent>(entity),
                                       transform);
      }
      else if (atmosphere.usePrimaryLight)
      {
        canComputeIBL = true;
        skyUpdated = skyAtm->submit(atmosphere, atmosphere, transform);
      }

      // Check to see if this entity has a dynamic sky light component for dynamic IBL.
      if (this->sceneECS.has<DynamicSkylightComponent>(entity) && canComputeIBL)
      {
        auto& iblComponent = this->sceneECS.get<DynamicSkylightComponent>(entity);
        dynIBL->submit(DynamicIBL(iblComponent.intensity, iblComponent.handle, atmosphere.handle), skyUpdated);
        iblApp->submitDynamicSkyIBL(DynamicIBL(iblComponent.intensity, iblComponent.handle, atmosphere.handle));
      }

      // Check to see if this entity has a dynamic skybox component.
      if (this->sceneECS.has<DynamicSkyboxComponent>(entity))
      {
        auto& dynSkybox = this->sceneECS.get<DynamicSkyboxComponent>(entity);
        skyboxApp->submit(atmosphere.handle, dynSkybox.sunSize, dynSkybox.intensity);
      }
    }
  }

  void
  Scene::onRenderRuntime()
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

    // Group together the transform, sky-atmosphere and directional light components.
    auto& passManager = Renderer3D::getPassManager();
    auto skyAtm = passManager.getRenderPass<SkyAtmospherePass>();
    auto dynIBL = passManager.getRenderPass<DynamicSkyIBLPass>();
    auto iblApp = passManager.getRenderPass<IBLApplicationPass>();
    auto skyboxApp = passManager.getRenderPass<SkyboxPass>();
    auto atmospheres = this->sceneECS.group<SkyAtmosphereComponent>(entt::get<TransformComponent>);
    for (auto entity : atmospheres)
    {
      auto [transform, atmosphere] = atmospheres.get<TransformComponent, SkyAtmosphereComponent>(entity);

      bool canComputeIBL = false;
      bool skyUpdated = false;
      if (this->sceneECS.has<DirectionalLightComponent>(entity) && !atmosphere.usePrimaryLight)
      {
        canComputeIBL = true;
        skyUpdated = skyAtm->submit(atmosphere, atmosphere, this->sceneECS.get<DirectionalLightComponent>(entity),
                                       transform);
      }
      else if (atmosphere.usePrimaryLight)
      {
        canComputeIBL = true;
        skyUpdated = skyAtm->submit(atmosphere, atmosphere, transform);
      }

      // Check to see if this entity has a dynamic sky light component for dynamic IBL.
      if (this->sceneECS.has<DynamicSkylightComponent>(entity) && canComputeIBL)
      {
        auto& iblComponent = this->sceneECS.get<DynamicSkylightComponent>(entity);
        dynIBL->submit(DynamicIBL(iblComponent.intensity, iblComponent.handle, atmosphere.handle), skyUpdated);
        iblApp->submitDynamicSkyIBL(DynamicIBL(iblComponent.intensity, iblComponent.handle, atmosphere.handle));
      }

      // Check to see if this entity has a dynamic skybox component.
      if (this->sceneECS.has<DynamicSkyboxComponent>(entity))
      {
        auto& dynSkybox = this->sceneECS.get<DynamicSkyboxComponent>(entity);
        skyboxApp->submit(atmosphere.handle, dynSkybox.sunSize, dynSkybox.intensity);
      }
    }
  }

  Entity
  Scene::getPrimaryCameraEntity()
  {
    // Loop over all the cameras with transform components to find the primary camera.
    auto cameras = this->sceneECS.group<CameraComponent>(entt::get<TransformComponent>);

    for (auto entity : cameras)
    {
      auto [camera, transform] = cameras.get<CameraComponent, TransformComponent>(entity);

      if (camera.isPrimary)
      {
        // Update the camera's attached matrices.
        glm::mat4 tMatrix = transform;
        camera.entCamera.front = glm::normalize(glm::vec3(tMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        camera.entCamera.position = glm::vec3(tMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        camera.entCamera.view = glm::lookAt(camera.entCamera.position,
                                            camera.entCamera.position + camera.entCamera.front,
                                            glm::vec3(0.0f, 1.0f, 0.0f));
        return Entity(entity, this);
      }
    }

    return Entity();
  }

  // Compute the global transform given a parent-child transform hierarchy.
  glm::mat4
  Scene::computeGlobalTransform(Entity entity)
  {
    if (entity.hasComponent<ParentEntityComponent>())
    {
      auto& parent = entity.getComponent<ParentEntityComponent>().parent;
      if (entity.hasComponent<TransformComponent>())
      {
        auto& localTransform = entity.getComponent<TransformComponent>();
        return computeGlobalTransform(parent) * static_cast<glm::mat4>(localTransform);
      }
      else
        return computeGlobalTransform(parent);
    }
    else
    {
      if (entity.hasComponent<TransformComponent>())
      {
        auto& localTransform = entity.getComponent<TransformComponent>();
        return static_cast<glm::mat4>(localTransform);
      }
      else
        return glm::mat4(1.0f);
    }
  }

  void
  Scene::updateAnimations(float dt)
  {
    // Get all the renderable components to update animations.
    auto renderables = this->sceneECS.view<RenderableComponent>();
    for (auto entity : renderables)
    {
      auto& renderable = renderables.get<RenderableComponent>(entity);
      renderable.animator.onUpdate(dt);
    }
  }
}
