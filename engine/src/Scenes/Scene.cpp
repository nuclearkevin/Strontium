#include "Scenes/Scene.h"

// Project includes.
#include "Scenes/Components.h"
#include "Scenes/Entity.h"

#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/RenderPassManager.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"
#include "Graphics/RenderPasses/IBLApplicationPass.h"
#include "Graphics/RenderPasses/DirectionalLightPass.h"
#include "Graphics/RenderPasses/SkyboxPass.h"
#include "Graphics/RenderPasses/PostProcessingPass.h"

namespace Strontium
{
  Scene::Scene(const std::string &filepath)
    : saveFilepath(filepath)
    , primaryCameraID(entt::null)
    , primaryDirLightID(entt::null)
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
    // Grab the required renderpasses for submission.
    auto& passManager = Renderer3D::getPassManager();
    auto shadow = passManager.getRenderPass<ShadowPass>();
    auto geomet = passManager.getRenderPass<GeometryPass>();
    auto dirApp = passManager.getRenderPass<DirectionalLightPass>();
    auto skyAtm = passManager.getRenderPass<SkyAtmospherePass>();
    auto dynIBL = passManager.getRenderPass<DynamicSkyIBLPass>();
    auto iblApp = passManager.getRenderPass<IBLApplicationPass>();
    auto skyboxApp = passManager.getRenderPass<SkyboxPass>();
    auto postProc = passManager.getRenderPass<PostProcessingPass>();

    bool drawOutline = false;

    // Group together the lights and submit them to the renderer.
    auto primaryLight = this->getPrimaryDirectionalEntity();
    auto dirLight = this->sceneECS.group<DirectionalLightComponent>(entt::get<TransformComponent>);
    for (auto entity : dirLight)
    {
      auto [directional, transform] = dirLight.get<DirectionalLightComponent, TransformComponent>(entity);

      // Skip over the primary light IF it cast shadows.
      if (primaryLight.entityID == entity && directional.castShadows)
      {
        dirApp->submitPrimary(directional, directional.castShadows, transform);
        skyAtm->submitPrimary(directional, directional.castShadows, transform);
        shadow->submitPrimary(directional, directional.castShadows, transform);
      }
      else
        dirApp->submit(directional, transform);
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
      glm::mat4 transformMatrix = static_cast<glm::mat4>(transform);

      // If a drawable item has a transform hierarchy, compute the global
      // transforms from local transforms.
      auto currentEntity = Entity(entity, this);
      if (currentEntity.hasComponent<ParentEntityComponent>())
        transformMatrix = computeGlobalTransform(currentEntity);

      bool selected = entity == selectedEntity;
      drawOutline = drawOutline || selected;

      // Submit the mesh + material + transform to the static deferred renderer queue.
      if (renderable && !renderable.animator.animationRenderable())
      {
        geomet->submit(static_cast<Model*>(renderable), renderable.materials, transformMatrix, 
                       static_cast<float>(entity), selected);
        shadow->submit(static_cast<Model*>(renderable), transformMatrix);
      }
      // If it has a valid animation, instead submit it to the dynamic deferred renderer queue.
      else if (renderable && renderable.animator.animationRenderable())
      {
        geomet->submit(static_cast<Model*>(renderable), &renderable.animator, 
                       renderable.materials, transformMatrix, static_cast<float>(entity), 
                       selected);
        shadow->submit(static_cast<Model*>(renderable), &renderable.animator, transformMatrix);
      }
    }

    // Group together the transform, sky-atmosphere and directional light components.
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

    postProc->getInternalDataBlock<PostProcessingPassDataBlock>()->drawOutline = drawOutline;
  }

  void
  Scene::onRenderRuntime()
  {
    // Grab the required renderpasses for submission.
    auto& passManager = Renderer3D::getPassManager();
    auto shadow = passManager.getRenderPass<ShadowPass>();
    auto geomet = passManager.getRenderPass<GeometryPass>();
    auto dirApp = passManager.getRenderPass<DirectionalLightPass>();
    auto skyAtm = passManager.getRenderPass<SkyAtmospherePass>();
    auto dynIBL = passManager.getRenderPass<DynamicSkyIBLPass>();
    auto iblApp = passManager.getRenderPass<IBLApplicationPass>();
    auto skyboxApp = passManager.getRenderPass<SkyboxPass>();

    // Group together the lights and submit them to the renderer.
    auto dirLight = this->sceneECS.group<DirectionalLightComponent>(entt::get<TransformComponent>);
    for (auto entity : dirLight)
    {
      auto [directional, transform] = dirLight.get<DirectionalLightComponent, TransformComponent>(entity);
      dirApp->submit(directional, transform);
    }

    auto primaryLight = this->getPrimaryDirectionalEntity();
    if (primaryLight)
    {
      auto [directional, transform] = this->sceneECS.get<DirectionalLightComponent, TransformComponent>(primaryLight.entityID);
      dirApp->submitPrimary(directional, directional.castShadows, transform);
      skyAtm->submitPrimary(directional, directional.castShadows, transform);
      shadow->submitPrimary(directional, directional.castShadows, transform);
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
      glm::mat4 transformMatrix = static_cast<glm::mat4>(transform);

      // If a drawable item has a transform hierarchy, compute the global
      // transforms from local transforms.
      auto currentEntity = Entity(entity, this);
      if (currentEntity.hasComponent<ParentEntityComponent>())
        transformMatrix = computeGlobalTransform(currentEntity);

      // Submit the mesh + material + transform to the static deferred renderer queue.
      if (renderable && !renderable.animator.animationRenderable())
      {
        geomet->submit(static_cast<Model*>(renderable), renderable.materials, transformMatrix);
        shadow->submit(static_cast<Model*>(renderable), transformMatrix);
      }
      // If it has a valid animation, instead submit it to the dynamic deferred renderer queue.
      else if (renderable && renderable.animator.animationRenderable())
      {
        geomet->submit(static_cast<Model*>(renderable), &renderable.animator, 
                       renderable.materials, transformMatrix);
        shadow->submit(static_cast<Model*>(renderable), &renderable.animator, transformMatrix);
      }
    }

    // Group together the transform, sky-atmosphere and directional light components.
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
  Scene::setPrimaryCameraEntity(Entity entity)
  {
    if (!entity)
      return;

    if (entity.hasComponent<CameraComponent>() 
        && entity.hasComponent<TransformComponent>())
      this->primaryCameraID = entity.entityID;
  }

  Entity 
  Scene::getPrimaryCameraEntity() 
  {
    if (!this->sceneECS.valid(this->primaryCameraID))
      return Entity();

    if (this->sceneECS.has<CameraComponent, TransformComponent>(this->primaryCameraID))
      return Entity(this->primaryCameraID, this); 
    else
      return Entity();
  }

  void 
  Scene::setPrimaryDirectionalEntity(Entity entity)
  {
    if (!entity)
      return;

    if (entity.hasComponent<DirectionalLightComponent>() 
        && entity.hasComponent<TransformComponent>())
      this->primaryDirLightID = entity.entityID;
  }

  Entity 
  Scene::getPrimaryDirectionalEntity()
  {
    if (!this->sceneECS.valid(this->primaryDirLightID))
      return Entity();

    if (this->sceneECS.has<DirectionalLightComponent, TransformComponent>(this->primaryDirLightID))
      return Entity(this->primaryDirLightID, this);
    else
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
