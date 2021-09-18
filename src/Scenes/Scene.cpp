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
  Scene::onUpdateEditor(float dt)
  {
    // Get all the renderable components to update animations.
    auto renderables = this->sceneECS.view<RenderableComponent>();
    for (auto entity : renderables)
    {
      auto& renderable = renderables.get<RenderableComponent>(entity);
      renderable.animator.onUpdate(dt);
    }
  }

  void
  Scene::onUpdateRuntime(float dt)
  {
    // Get all the renderable components to update animations.
    auto renderables = this->sceneECS.view<RenderableComponent>();
    for (auto entity : renderables)
    {
      auto& renderable = renderables.get<RenderableComponent>(entity);
      renderable.animator.onUpdate(dt);
    }
  }

  void
  Scene::onRenderEditor(Entity selectedEntity)
  {
    // Prepare the ambient component.
    auto ambLight = this->sceneECS.group<AmbientComponent>(entt::get<TransformComponent>);
    for (auto entity : ambLight)
    {
      auto [ambient, transform] = ambLight.get<AmbientComponent, TransformComponent>(entity);
      if (ambient.ambient->getDrawingType() == MapType::DynamicSky)
      {
        if (ambient.ambient->getDynamicSkyType() == DynamicSkyType::Preetham)
        {
          DynamicSkyCommonParams& skyParams = ambient.ambient->getSkyModelParams(DynamicSkyType::Preetham);
          PreethamSkyParams preethamParams = *(static_cast<PreethamSkyParams*>(&skyParams));

          glm::mat4 rotation = glm::transpose(glm::inverse((glm::mat4) transform));
          preethamParams.sunPos = glm::vec3(rotation * glm::vec4(0.0, 1.0, 0.0, 0.0f));
          preethamParams.sunPos.z *= -1.0f;

          ambient.ambient->setSkyModelParams(static_cast<DynamicSkyCommonParams*>(&preethamParams));
        }
        else if (ambient.ambient->getDynamicSkyType() == DynamicSkyType::Hillaire)
        {
          DynamicSkyCommonParams& skyParams = ambient.ambient->getSkyModelParams(DynamicSkyType::Hillaire);
          HillaireSkyParams hillaireParams = *(static_cast<HillaireSkyParams*>(&skyParams));

          glm::mat4 rotation = glm::transpose(glm::inverse((glm::mat4) transform));
          hillaireParams.sunPos = glm::vec3(rotation * glm::vec4(0.0, 1.0, 0.0, 0.0f));
          hillaireParams.sunPos *= -1.0f;

          ambient.ambient->setSkyModelParams(static_cast<DynamicSkyCommonParams*>(&hillaireParams));
        }
      }
    }

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

      // Submit the mesh + material + transform to the static deferred renderer queue.
      if (renderable && !renderable.animator.animationRenderable())
        Renderer3D::submit(renderable, renderable, transformMatrix,
                           static_cast<float>(entity), selected);
      // If it has a valid animation, instead submit it to the dynamic deferred renderer queue.
      else if (renderable && renderable.animator.animationRenderable())
        Renderer3D::submit(renderable, &renderable.animator, renderable,
                           transformMatrix, static_cast<float>(entity), selected);
    }
  }

  void
  Scene::onRenderRuntime()
  {
    // Prepare the ambient component.
    auto ambLight = this->sceneECS.group<AmbientComponent>(entt::get<TransformComponent>);
    for (auto entity : ambLight)
    {
      auto [ambient, transform] = ambLight.get<AmbientComponent, TransformComponent>(entity);
      if (ambient.ambient->getDrawingType() == MapType::DynamicSky)
      {
        if (ambient.ambient->getDynamicSkyType() == DynamicSkyType::Preetham)
        {
          DynamicSkyCommonParams& skyParams = ambient.ambient->getSkyModelParams(DynamicSkyType::Preetham);
          PreethamSkyParams preethamParams = *(static_cast<PreethamSkyParams*>(&skyParams));

          glm::mat4 rotation = glm::transpose(glm::inverse((glm::mat4) transform));
          preethamParams.sunPos = glm::vec3(rotation * glm::vec4(0.0, 1.0, 0.0, 0.0f));
          preethamParams.sunPos.z *= -1.0f;

          ambient.ambient->setSkyModelParams(static_cast<DynamicSkyCommonParams*>(&preethamParams));
        }
        else if (ambient.ambient->getDynamicSkyType() == DynamicSkyType::Hillaire)
        {
          DynamicSkyCommonParams& skyParams = ambient.ambient->getSkyModelParams(DynamicSkyType::Hillaire);
          HillaireSkyParams hillaireParams = *(static_cast<HillaireSkyParams*>(&skyParams));

          glm::mat4 rotation = glm::transpose(glm::inverse((glm::mat4) transform));
          hillaireParams.sunPos = glm::vec3(rotation * glm::vec4(0.0, 1.0, 0.0, 0.0f));
          hillaireParams.sunPos *= -1.0f;

          ambient.ambient->setSkyModelParams(static_cast<DynamicSkyCommonParams*>(&hillaireParams));
        }
      }
    }

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

      // Submit the mesh + material + transform to the static deferred renderer queue.
      if (renderable && !renderable.animator.animationRenderable())
        Renderer3D::submit(renderable, renderable, transformMatrix,
                           static_cast<float>(entity));
      // If it has a valid animation, instead submit it to the dynamic deferred renderer queue.
      else if (renderable && renderable.animator.animationRenderable())
        Renderer3D::submit(renderable, &renderable.animator, renderable,
                           transformMatrix, static_cast<float>(entity));
    }
  }

  Entity
  Scene::getPrimaryCameraEntity()
  {
    // Loop over all the cameras with transform components to find the primary camera.
    auto cameras = this->sceneECS.group<CameraComponent>(entt::get<TransformComponent>);

    Entity lastCamera = Entity();
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
        camera.entCamera.projection = glm::perspective(camera.entCamera.fov,
                                                       1.0f, camera.entCamera.near,
                                                       camera.entCamera.far);
        return Entity(entity, this);
      }

      lastCamera = Entity(entity, this);
    }

    // If we got here, there is no primary camera. Using the last camera in the
    // pool as a last resort. If there is no last camera, return a null entity.
    if (lastCamera)
    {
      auto& transform = lastCamera.getComponent<TransformComponent>();
      glm::mat4 tMatrix = transform;

      auto& camera = lastCamera.getComponent<CameraComponent>();
      camera.entCamera.front = glm::normalize(glm::vec3(tMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
      camera.entCamera.position = glm::vec3(tMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
      camera.entCamera.view = glm::lookAt(camera.entCamera.position,
                                          camera.entCamera.position + camera.entCamera.front,
                                          glm::vec3(0.0f, 1.0f, 0.0f));
      camera.entCamera.projection = glm::perspective(camera.entCamera.fov,
                                                     1.0f, camera.entCamera.near,
                                                     camera.entCamera.far);
    }
    return lastCamera;
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
