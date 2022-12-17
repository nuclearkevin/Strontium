#include "Scenes/Scene.h"

// Project includes.
#include "Core/Application.h"

#include "Assets/AssetManager.h"
#include "Assets/ModelAsset.h"

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
#include "Graphics/RenderPasses/GodrayPass.h"

#include "Graphics/RenderPasses/WireframePass.h"

#include "PhysicsEngine/PhysicsEngine.h"

// GLM includes.
#include "glm/gtx/matrix_decompose.hpp"

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
  Scene::initPhysics()
  {
    // Group together and fetch all the entities that have 
    // transform + rigid body + sphere collider components.
    {
      auto sphereColliders = this->sceneECS.group<SphereColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : sphereColliders)
        PhysicsEngine::addActor(Entity(entity, this));
    }

    // Group together and fetch all the entities that have 
    // transform + rigid body + box collider components.
    {
      auto boxColliders = this->sceneECS.group<BoxColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : boxColliders)
        PhysicsEngine::addActor(Entity(entity, this));
    }

    // Group together and fetch all the entities that have 
    // transform + rigid body + cylinder collider components.
    {
      auto cylinderColliders = this->sceneECS.group<CylinderColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : cylinderColliders)
        PhysicsEngine::addActor(Entity(entity, this));
    }

    // Group together and fetch all the entities that have 
    // transform + rigid body + capsule collider components.
    {
      auto capsuleColliders = this->sceneECS.group<CapsuleColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : capsuleColliders)
        PhysicsEngine::addActor(Entity(entity, this));
    }

    PhysicsEngine::onSimulationBegin();
  }

  void 
  Scene::shutdownPhysics()
  {
    PhysicsEngine::onSimulationEnd();
  }

  void 
  Scene::simulatePhysics(float dt)
  {
    //----------------------------------------------------------------------------
    // Pre-physics steps. Update the physics system if needed.
    //----------------------------------------------------------------------------
    

    //----------------------------------------------------------------------------
    // Perform the actual physics steps.
    //----------------------------------------------------------------------------
    PhysicsEngine::onUpdate(dt);

    //----------------------------------------------------------------------------
    // Post-physics steps. Fetch positions and orientations from the physics 
    // system to update the transforms.
    //----------------------------------------------------------------------------
    // Group together and fetch all the entities that have 
    // transform + rigid body + sphere collider components.
    {
      auto sphereColliders = this->sceneECS.group<SphereColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : sphereColliders)
      {
        auto& actor = PhysicsEngine::getActor(Entity(entity, this));
        if (!actor.isValid())
          continue;

        auto transformData = actor.getUpdatedTransformData();
        auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
        auto& collider = this->sceneECS.get<SphereColliderComponent>(entity);

        auto prePhysicsTransform = this->computeGlobalTransform(Entity(entity, this));
        auto prePhysicsNoLocal = prePhysicsTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
        auto localSpaceCenter = glm::vec3(glm::inverse(prePhysicsNoLocal) * glm::vec4(transformData.translation, 1.0f));

        auto globalRotation = this->computeGlobalRotation(Entity(entity, this));
        auto prePhysicsNoLocalRot = globalRotation * glm::normalize(glm::inverse(glm::quat(localTransform.rotation)));
        auto newLocalRot = glm::quat(transformData.rotation) * glm::inverse(glm::normalize(prePhysicsNoLocalRot));
        auto localRotation = glm::toMat4(newLocalRot);

        localTransform.translation = localSpaceCenter - glm::vec3(localRotation * glm::vec4(collider.offset, 1.0f));
        localTransform.rotation = glm::eulerAngles(newLocalRot);
      }
    }

    // Group together and fetch all the entities that have 
    // transform + rigid body + box collider components.
    {
      auto boxColliders = this->sceneECS.group<BoxColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : boxColliders)
      {
        auto& actor = PhysicsEngine::getActor(Entity(entity, this));
        if (!actor.isValid())
          continue;

        auto transformData = actor.getUpdatedTransformData();
        auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
        auto& collider = this->sceneECS.get<BoxColliderComponent>(entity);

        auto prePhysicsTransform = this->computeGlobalTransform(Entity(entity, this));
        auto prePhysicsNoLocal = prePhysicsTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
        auto localSpaceCenter = glm::vec3(glm::inverse(prePhysicsNoLocal) * glm::vec4(transformData.translation, 1.0f));

        auto globalRotation = this->computeGlobalRotation(Entity(entity, this));
        auto prePhysicsNoLocalRot = globalRotation * glm::normalize(glm::inverse(glm::quat(localTransform.rotation)));
        auto newLocalRot = glm::quat(transformData.rotation) * glm::inverse(glm::normalize(prePhysicsNoLocalRot));
        auto localRotation = glm::toMat4(newLocalRot);

        localTransform.translation = localSpaceCenter - glm::vec3(localRotation * glm::vec4(collider.offset, 1.0f));
        localTransform.rotation = glm::eulerAngles(newLocalRot);
      }
    }

    // Group together and fetch all the entities that have 
    // transform + rigid body + cylinder collider components.
    {
      auto cylinderColliders = this->sceneECS.group<CylinderColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : cylinderColliders)
      {
        auto& actor = PhysicsEngine::getActor(Entity(entity, this));
        if (!actor.isValid())
          continue;

        auto transformData = actor.getUpdatedTransformData();
        auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
        auto& collider = this->sceneECS.get<CylinderColliderComponent>(entity);

        auto prePhysicsTransform = this->computeGlobalTransform(Entity(entity, this));
        auto prePhysicsNoLocal = prePhysicsTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
        auto localSpaceCenter = glm::vec3(glm::inverse(prePhysicsNoLocal) * glm::vec4(transformData.translation, 1.0f));

        auto globalRotation = this->computeGlobalRotation(Entity(entity, this));
        auto prePhysicsNoLocalRot = globalRotation * glm::normalize(glm::inverse(glm::quat(localTransform.rotation)));
        auto newLocalRot = glm::quat(transformData.rotation) * glm::inverse(glm::normalize(prePhysicsNoLocalRot));
        auto localRotation = glm::toMat4(newLocalRot);

        localTransform.translation = localSpaceCenter - glm::vec3(localRotation * glm::vec4(collider.offset, 1.0f));
        localTransform.rotation = glm::eulerAngles(newLocalRot);
      }
    }

    // Group together and fetch all the entities that have 
    // transform + rigid body + capsule collider components.
    {
      auto capsuleColliders = this->sceneECS.group<CapsuleColliderComponent>(entt::get<TransformComponent, RigidBody3DComponent>);
      for (auto entity : capsuleColliders)
      {
        auto& actor = PhysicsEngine::getActor(Entity(entity, this));
        if (!actor.isValid())
          continue;

        auto transformData = actor.getUpdatedTransformData();
        auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
        auto& collider = this->sceneECS.get<CapsuleColliderComponent>(entity);

        auto prePhysicsTransform = this->computeGlobalTransform(Entity(entity, this));
        auto prePhysicsNoLocal = prePhysicsTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
        auto localSpaceCenter = glm::vec3(glm::inverse(prePhysicsNoLocal) * glm::vec4(transformData.translation, 1.0f));

        auto globalRotation = this->computeGlobalRotation(Entity(entity, this));
        auto prePhysicsNoLocalRot = globalRotation * glm::normalize(glm::inverse(glm::quat(localTransform.rotation)));
        auto newLocalRot = glm::quat(transformData.rotation) * glm::inverse(glm::normalize(prePhysicsNoLocalRot));
        auto localRotation = glm::toMat4(newLocalRot);

        localTransform.translation = localSpaceCenter - glm::vec3(localRotation * glm::vec4(collider.offset, 1.0f));
        localTransform.rotation = glm::eulerAngles(newLocalRot);
      }
    }
  }

  void
  Scene::onUpdateEditor(float dt)
  {
    // Get all the renderable components to update animations.
    auto renderables = this->sceneECS.view<RenderableComponent>();
    for (auto entity : renderables)
      renderables.get<RenderableComponent>(entity).animator.onUpdate(dt);
  }

  void
  Scene::onUpdateRuntime(float dt)
  {
    // Get all the renderable components to update animations.
    auto renderables = this->sceneECS.view<RenderableComponent>();
    for (auto entity : renderables)
      renderables.get<RenderableComponent>(entity).animator.onUpdate(dt);
  }

  void
  Scene::onRenderEditor(float viewportAspect, Entity selectedEntity)
  {
    // Grab the asset cache.
    auto& assetCache = Application::getInstance()->getAssetCache();

    // Grab the required renderpasses for submission.
    auto& passManager = Renderer3D::getPassManager();
    auto shadow = passManager.getRenderPass<ShadowPass>();
    auto geomet = passManager.getRenderPass<GeometryPass>();
    auto dirApp = passManager.getRenderPass<DirectionalLightPass>();
    auto skyAtm = passManager.getRenderPass<SkyAtmospherePass>();
    auto godray = passManager.getRenderPass<GodrayPass>();
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
      if (primaryLight.entityID == entity)
      {
        RendererDataHandle attachedSky = -1;
        if (this->sceneECS.has<SkyAtmosphereComponent>(entity))
          attachedSky = this->sceneECS.get<SkyAtmosphereComponent>(entity).handle;

        dirApp->submitPrimary(directional, directional.castShadows, transform, attachedSky);
        skyAtm->submitPrimary(directional, directional.castShadows, transform);
        shadow->submitPrimary(directional, directional.castShadows, transform);
      }
      else
      {
        RendererDataHandle attachedSky = -1;
        if (this->sceneECS.has<SkyAtmosphereComponent>(entity))
          attachedSky = this->sceneECS.get<SkyAtmosphereComponent>(entity).handle;

        dirApp->submit(directional, transform, attachedSky);
      }
    }

    // TODO: Point light pass.
    /*
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
    }
    */

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
      auto modelAsset = assetCache.get<ModelAsset>(renderable.meshName);
      if (modelAsset && !renderable.animator.animationRenderable())
      {
        geomet->submit(modelAsset->getModel(), renderable.materials, transformMatrix,
                       static_cast<float>(entity), selected);
        shadow->submit(modelAsset->getModel(), transformMatrix);
      }
      // If it has a valid animation, instead submit it to the dynamic deferred renderer queue.
      else if (modelAsset && renderable.animator.animationRenderable())
      {
        geomet->submit(modelAsset->getModel(), &renderable.animator,
                       renderable.materials, transformMatrix, static_cast<float>(entity), 
                       selected);
        shadow->submit(modelAsset->getModel(), &renderable.animator, transformMatrix);
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

    // Group together and submit fog volumes.
    // Box.
    auto obbFog = this->sceneECS.group<BoxFogVolumeComponent>(entt::get<TransformComponent>);
    for (auto entity : obbFog)
    {
      auto [transform, boxFog] = obbFog.get<TransformComponent, BoxFogVolumeComponent>(entity);
      godray->submit(OBBFogVolume(boxFog.phase, boxFog.density, boxFog.absorption, 
                                  boxFog.mieScattering, boxFog.emission, static_cast<glm::mat4>(transform)));
    }
    // Sphere.
    auto sFog = this->sceneECS.group<SphereFogVolumeComponent>(entt::get<TransformComponent>);
    for (auto entity : sFog)
    {
      auto [transform, sphereFog] = sFog.get<TransformComponent, SphereFogVolumeComponent>(entity);
      godray->submit(SphereFogVolume(sphereFog.phase, sphereFog.density, sphereFog.absorption,
                                     sphereFog.mieScattering, sphereFog.emission, transform.translation, 
                                     sphereFog.radius));
    }

    postProc->getInternalDataBlock<PostProcessingPassDataBlock>()->drawOutline = drawOutline;
  }

  void
  Scene::onRenderRuntime(float viewportAspect)
  {
    // Grab the asset cache.
    auto& assetCache = Application::getInstance()->getAssetCache();

    // Grab the required renderpasses for submission.
    auto& passManager = Renderer3D::getPassManager();
    auto shadow = passManager.getRenderPass<ShadowPass>();
    auto geomet = passManager.getRenderPass<GeometryPass>();
    auto dirApp = passManager.getRenderPass<DirectionalLightPass>();
    auto skyAtm = passManager.getRenderPass<SkyAtmospherePass>();
    auto godray = passManager.getRenderPass<GodrayPass>();
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
      if (primaryLight.entityID == entity)
      {
        RendererDataHandle attachedSky = -1;
        if (this->sceneECS.has<SkyAtmosphereComponent>(entity))
          attachedSky = this->sceneECS.get<SkyAtmosphereComponent>(entity).handle;

        dirApp->submitPrimary(directional, directional.castShadows, transform, attachedSky);
        skyAtm->submitPrimary(directional, directional.castShadows, transform);
        shadow->submitPrimary(directional, directional.castShadows, transform);
      }
      else
      {
        RendererDataHandle attachedSky = -1;
        if (this->sceneECS.has<SkyAtmosphereComponent>(entity))
          attachedSky = this->sceneECS.get<SkyAtmosphereComponent>(entity).handle;

        dirApp->submit(directional, transform, attachedSky);
      }
    }

    // TODO: Point light pass.
    /*
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
    }
    */

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
      auto modelAsset = assetCache.get<ModelAsset>(renderable.meshName);
      if (modelAsset && !renderable.animator.animationRenderable())
      {
        geomet->submit(modelAsset->getModel(), renderable.materials, transformMatrix);
        shadow->submit(modelAsset->getModel(), transformMatrix);
      }
      // If it has a valid animation, instead submit it to the dynamic deferred renderer queue.
      else if (modelAsset && renderable.animator.animationRenderable())
      {
        geomet->submit(modelAsset->getModel(), &renderable.animator,
                       renderable.materials, transformMatrix);
        shadow->submit(modelAsset->getModel(), &renderable.animator, transformMatrix);
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

    // Group together and submit fog volumes.
    // Box.
    auto obbFog = this->sceneECS.group<BoxFogVolumeComponent>(entt::get<TransformComponent>);
    for (auto entity : obbFog)
    {
      auto [transform, boxFog] = obbFog.get<TransformComponent, BoxFogVolumeComponent>(entity);
      godray->submit(OBBFogVolume(boxFog.phase, boxFog.density, boxFog.absorption, 
                                  boxFog.mieScattering, boxFog.emission, static_cast<glm::mat4>(transform)));
    }
    // Sphere.
    auto sFog = this->sceneECS.group<SphereFogVolumeComponent>(entt::get<TransformComponent>);
    for (auto entity : sFog)
    {
      auto [transform, sphereFog] = sFog.get<TransformComponent, SphereFogVolumeComponent>(entity);
      godray->submit(SphereFogVolume(sphereFog.phase, sphereFog.density, sphereFog.absorption,
                                     sphereFog.mieScattering, sphereFog.emission, transform.translation, 
                                     sphereFog.radius));
    }

    postProc->getInternalDataBlock<PostProcessingPassDataBlock>()->drawOutline = false;
  }

  void 
  Scene::onRenderDebug(float viewportAspect)
  {
    auto& debugRendererData = DebugRenderer::getStorage();
    auto& debugPassManager = DebugRenderer::getPassManager();
    auto wireframePass = debugPassManager.getRenderPass<WireframePass>();
    
    // Group together and fetch all the entities that have 
    // transform + camera components.
    {
      auto cameras = this->sceneECS.group<CameraComponent>(entt::get<TransformComponent>);
      for (auto entity : cameras)
      {
        auto& camera = this->sceneECS.get<CameraComponent>(entity);
        if (camera.visualize)
        {
          auto matrix = this->computeGlobalTransform(Entity(entity, this));
          Camera& cam = camera.entCamera;

          cam.position = glm::vec3(matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
          cam.front = glm::normalize(glm::vec3(matrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

          cam.view = glm::lookAt(cam.position, cam.position + cam.front, glm::vec3(0.0f, 1.0f, 0.0f));

          cam.projection = glm::perspective(cam.fov, viewportAspect, cam.near, cam.far);
          cam.invViewProj = glm::inverse(cam.projection * cam.view);

          wireframePass->submitFrustum(buildCameraFrustum(cam), glm::vec3(0.0f, 1.0f, 0.0f));
        }
      }
    }

    // Group together and fetch all the entities that have 
    // transform + sphere collider components.
    {
      auto sphereColliders = this->sceneECS.group<SphereColliderComponent>(entt::get<TransformComponent>);
      for (auto entity : sphereColliders)
      {
        auto& collider = this->sceneECS.get<SphereColliderComponent>(entity);
        if (collider.visualize || debugRendererData.visualizeAllColliders)
        {
          auto globalTransform = this->computeGlobalTransform(Entity(entity, this));
          auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
          globalTransform = globalTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
          auto matrix = globalTransform * glm::translate(localTransform.translation) * glm::toMat4(glm::quat(localTransform.rotation));

          wireframePass->submitSphere(Sphere(glm::vec3(matrix * glm::vec4(collider.offset, 1.0f)),
                                             collider.radius), 
                                      glm::vec3(0.0f, 1.0f, 0.0f));
        }
      }
    }

    // Group together and fetch all the entities that have 
    // transform + box collider components.
    {
      auto boxColliders = this->sceneECS.group<BoxColliderComponent>(entt::get<TransformComponent>);
      for (auto entity : boxColliders)
      {
        auto& collider = this->sceneECS.get<BoxColliderComponent>(entity);
        if (collider.visualize || debugRendererData.visualizeAllColliders)
        {
          auto globalTransform = this->computeGlobalTransform(Entity(entity, this));
          auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
          globalTransform = globalTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
          auto matrix = globalTransform * glm::translate(localTransform.translation) * glm::toMat4(glm::quat(localTransform.rotation));

          auto globalRotation = this->computeGlobalRotation(Entity(entity, this));

          wireframePass->submitOrientedBox(OrientedBoundingBox(glm::vec3(matrix * glm::vec4(collider.offset, 1.0f)),
                                                               collider.extents, 
                                                               glm::quat(globalRotation)),
                                            glm::vec3(0.0f, 1.0f, 0.0f));
        }
      }
    }

    // Group together and fetch all the entities that have 
    // transform + cylinder collider components.
    {
      auto cylinderColliders = this->sceneECS.group<CylinderColliderComponent>(entt::get<TransformComponent>);
      for (auto entity : cylinderColliders)
      {
        auto& collider = this->sceneECS.get<CylinderColliderComponent>(entity);
        if (collider.visualize || debugRendererData.visualizeAllColliders)
        {
          auto globalTransform = this->computeGlobalTransform(Entity(entity, this));
          auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
          globalTransform = globalTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
          auto matrix = globalTransform * glm::translate(localTransform.translation) * glm::toMat4(glm::quat(localTransform.rotation));

          auto globalRotation = this->computeGlobalRotation(Entity(entity, this));

          wireframePass->submitCylinder(Cylinder(glm::vec3(matrix * glm::vec4(collider.offset, 1.0f)), 
                                                 collider.halfHeight, collider.radius, glm::quat(globalRotation)),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
        }
      }
    }

    // Group together and fetch all the entities that have 
    // transform + capsule collider components.
    {
      auto capsuleColliders = this->sceneECS.group<CapsuleColliderComponent>(entt::get<TransformComponent>);
      for (auto entity : capsuleColliders)
      {
        auto& collider = this->sceneECS.get<CapsuleColliderComponent>(entity);
        if (collider.visualize || debugRendererData.visualizeAllColliders)
        {
          auto globalTransform = this->computeGlobalTransform(Entity(entity, this));
          auto& localTransform = this->sceneECS.get<TransformComponent>(entity);
          globalTransform = globalTransform * glm::inverse(static_cast<glm::mat4>(localTransform));
          auto matrix = globalTransform * glm::translate(localTransform.translation) * glm::toMat4(glm::quat(localTransform.rotation));

          auto globalRotation = this->computeGlobalRotation(Entity(entity, this));

          wireframePass->submitCapsule(Capsule(glm::vec3(matrix * glm::vec4(collider.offset, 1.0f)), 
                                               collider.halfHeight, collider.radius, glm::quat(globalRotation)),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
        }
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
        return static_cast<glm::mat4>(entity.getComponent<TransformComponent>());
      else
        return glm::mat4(1.0f);
    }
  }

  glm::quat 
  Scene::computeGlobalRotation(Entity entity)
  {
    if (entity.hasComponent<ParentEntityComponent>())
    {
      auto& parent = entity.getComponent<ParentEntityComponent>().parent;
      if (entity.hasComponent<TransformComponent>())
      {
        auto& localRotation = glm::quat(entity.getComponent<TransformComponent>().rotation);
        return computeGlobalRotation(parent) * localRotation;
      }
      else
        return computeGlobalRotation(parent);
    }
    else
    {
      if (entity.hasComponent<TransformComponent>())
        return glm::quat(entity.getComponent<TransformComponent>().rotation);
      else
        return glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    }
  }

  // Helper functions to copy a component from source into destination.
  template <typename T>
  void 
  copyComponent(Entity source, Entity destination)
  {
    if (source.hasComponent<T>())
    {
      auto& comp = destination.addComponent<T>();
      comp = source.getComponent<T>();
    }
  }

  // Specialize for the name component.
  template <>
  void 
  copyComponent<NameComponent>(Entity source, Entity destination)
  {
    destination.getComponent<NameComponent>() = source.getComponent<NameComponent>();
  }

  // Specialize for components that use renderer handles.
  template <>
  void 
  copyComponent<SkyAtmosphereComponent>(Entity source, Entity destination)
  {
    if (source.hasComponent<SkyAtmosphereComponent>())
    {
      auto& passManager = Renderer3D::getPassManager();
      auto skyAtm = passManager.getRenderPass<SkyAtmospherePass>();

      auto& dest = destination.addComponent<SkyAtmosphereComponent>();
      auto& src = source.getComponent<SkyAtmosphereComponent>();
      
      skyAtm->deleteRendererData(dest.handle);
      
      dest.mieAbs = src.mieAbs;
      dest.mieScat = src.mieScat;
      dest.rayleighAbs = src.rayleighAbs;
      dest.rayleighScat = src.rayleighScat;
      dest.ozoneAbs = src.ozoneAbs;
      dest.planetAlbedo = src.planetAlbedo;
      dest.planetAtmRadius = src.planetAtmRadius;
      dest.usePrimaryLight = src.usePrimaryLight;
      dest.handle = src.handle;
    }
  }

  template <>
  void 
  copyComponent<DynamicSkylightComponent>(Entity source, Entity destination)
  {
    if (source.hasComponent<DynamicSkylightComponent>())
    {
      auto& passManager = Renderer3D::getPassManager();
      auto dynIBL = passManager.getRenderPass<DynamicSkyIBLPass>();

      auto& dest = destination.addComponent<DynamicSkylightComponent>();
      auto& src = source.getComponent<DynamicSkylightComponent>();

      dynIBL->deleteRendererData(dest.handle);

      dest.intensity = src.intensity;
      dest.handle = src.handle;
    }
  }

  void 
  copyComponents(Entity source, Entity destination)
  {
    copyComponent<NameComponent>(source, destination);
    copyComponent<TransformComponent>(source, destination);
    copyComponent<RenderableComponent>(source, destination);
    copyComponent<CameraComponent>(source, destination);
    copyComponent<SkyAtmosphereComponent>(source, destination);
    copyComponent<DynamicSkyboxComponent>(source, destination);
    copyComponent<DirectionalLightComponent>(source, destination);
    copyComponent<PointLightComponent>(source, destination);
    copyComponent<DynamicSkylightComponent>(source, destination);
    copyComponent<SphereColliderComponent>(source, destination);
    copyComponent<BoxColliderComponent>(source, destination);
    copyComponent<CylinderColliderComponent>(source, destination);
    copyComponent<CapsuleColliderComponent>(source, destination);
    copyComponent<RigidBody3DComponent>(source, destination);
    copyComponent<BoxFogVolumeComponent>(source, destination);
  }

  void 
  traverseHierarchyCopy(Entity sourceParent, Entity destinationParent)
  {
    if (sourceParent.hasComponent<ChildEntityComponent>())
    {
      Scene* sourceScene = static_cast<Scene*>(sourceParent);
      Scene* destinationScene = static_cast<Scene*>(destinationParent);

      auto& sourceChildEntityComponent = sourceParent.getComponent<ChildEntityComponent>();
      auto& destinationChildEntityComponent = destinationParent.addComponent<ChildEntityComponent>();

      for (auto& sourceChild : sourceChildEntityComponent.children)
      {
        // Create a new child entity and add it to the new parent's list of children.
        auto newDestinationChildEntity = destinationScene->createEntity();
        destinationChildEntityComponent.children.push_back(newDestinationChildEntity);

        // Create a parent component on the child and add it's parent.
        auto& destinationParentComponent = newDestinationChildEntity.addComponent<ParentEntityComponent>();
        destinationParentComponent.parent = destinationParent;

        // Traverse the hierarchy.
        traverseHierarchyCopy(sourceChild, newDestinationChildEntity);
      }
    }

    // Copy over the components.
    copyComponents(sourceParent, destinationParent);

    // Set the primary entities.
    if (sourceParent == static_cast<Scene*>(sourceParent)->getPrimaryCameraEntity())
      static_cast<Scene*>(destinationParent)->setPrimaryCameraEntity(destinationParent);
    if (sourceParent == static_cast<Scene*>(sourceParent)->getPrimaryDirectionalEntity())
      static_cast<Scene*>(destinationParent)->setPrimaryDirectionalEntity(destinationParent);
  }

  void 
  Scene::copyForRuntime(Scene& other)
  {
    this->sceneECS = entt::registry();

    auto currentScene = this;
    auto otherScene = &other;

    auto copyHierarchy = [currentScene, otherScene](entt::entity entity, ChildEntityComponent &comp)
    {
      if (Entity(entity, otherScene).hasComponent<ParentEntityComponent>())
        return;

      auto& destinationEntity = currentScene->createEntity();
      auto& sourceEntity = Entity(entity, otherScene);

      traverseHierarchyCopy(sourceEntity, destinationEntity);
    };

    other.sceneECS.view<ChildEntityComponent>().each(copyHierarchy);

    auto copyFlattened = [currentScene, otherScene](entt::entity entity, NameComponent &comp)
    {
      auto destinationEntity = currentScene->createEntity();
      auto sourceEntity = Entity(entity, otherScene);

      copyComponents(sourceEntity, destinationEntity);

      // Set the primary entities.
      if (sourceEntity == static_cast<Scene*>(sourceEntity)->getPrimaryCameraEntity())
        static_cast<Scene*>(destinationEntity)->setPrimaryCameraEntity(destinationEntity);
      if (sourceEntity == static_cast<Scene*>(sourceEntity)->getPrimaryDirectionalEntity())
        static_cast<Scene*>(destinationEntity)->setPrimaryDirectionalEntity(destinationEntity);
    };

    other.sceneECS.view<NameComponent>(entt::exclude<ChildEntityComponent, ParentEntityComponent>).each(copyFlattened);
  }

  // Helper functions to delete a component without changing renderer handles.
  template <typename T>
  void 
  deleteComponent(Entity source)
  {
    if (source.hasComponent<T>())
      source.removeComponent<T>();
  }

  // Specialize components that use renderer handles.
  template <>
  void 
  deleteComponent<SkyAtmosphereComponent>(Entity source)
  {
    if (source.hasComponent<SkyAtmosphereComponent>())
    {
      auto& component = source.getComponent<SkyAtmosphereComponent>();
      component.handle = -1;
      source.removeComponent<SkyAtmosphereComponent>();
    }
  }

  template <>
  void 
  deleteComponent<DynamicSkylightComponent>(Entity source)
  {
    if (source.hasComponent<DynamicSkylightComponent>())
    {
      auto& component = source.getComponent<DynamicSkylightComponent>();
      component.handle = -1;
      source.removeComponent<DynamicSkylightComponent>();
    }
  }

  void 
  deleteComponents(Entity source)
  {
    deleteComponent<NameComponent>(source);
    deleteComponent<ParentEntityComponent>(source);
    deleteComponent<ChildEntityComponent>(source);
    deleteComponent<TransformComponent>(source);
    deleteComponent<RenderableComponent>(source);
    deleteComponent<CameraComponent>(source);
    deleteComponent<SkyAtmosphereComponent>(source);
    deleteComponent<DynamicSkyboxComponent>(source);
    deleteComponent<DirectionalLightComponent>(source);
    deleteComponent<PointLightComponent>(source);
    deleteComponent<DynamicSkylightComponent>(source);
    deleteComponent<SphereColliderComponent>(source);
    deleteComponent<BoxColliderComponent>(source);
    deleteComponent<CylinderColliderComponent>(source);
    deleteComponent<CapsuleColliderComponent>(source);
    deleteComponent<RigidBody3DComponent>(source);
    deleteComponent<BoxFogVolumeComponent>(source);

    Scene* scene = static_cast<Scene*>(source);
    scene->deleteEntity(source);
  }

  void 
  Scene::clearForRuntime()
  {
    this->clearPrimaryCameraEntity();
    this->clearPrimaryDirectionalEntity();

    auto function = [this](entt::entity entity)
    {
      deleteComponents(Entity(entity, this));
    };

    this->sceneECS.each(function);
    this->sceneECS.clear();
    this->sceneECS = entt::registry();
  }
}
