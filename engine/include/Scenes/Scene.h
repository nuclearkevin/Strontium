#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/ShadingPrimatives.h"

// Entity component system includes.
#include "entt.hpp"

namespace Strontium
{
  class Entity;

  class Scene
  {
  public:
    Scene(const std::string &filepath = "");
    ~Scene();

    Entity createEntity(const std::string& name = "New Entity");
    Entity createEntity(uint entityID, const std::string& name = "New Entity");
    void recurseDeleteEntity(Entity entity);
    void deleteEntity(Entity entity);

    // Physics functions.
    void initPhysics();
    void shutdownPhysics();
    void simulatePhysics(float dt);

    // Update functions.
    void onUpdateEditor(float dt);
    void onUpdateRuntime(float dt);

    // Rendering functions.
    void onRenderEditor(float viewportAspect, Entity selectedEntity);
    void onRenderRuntime(float viewportAspect);
    void onRenderDebug(float viewportAspect);

    // Get and set primary entities.
    Entity getPrimaryCameraEntity();
    void setPrimaryCameraEntity(Entity entity);
    void clearPrimaryCameraEntity() { this->primaryCameraID = entt::null; }

    Entity getPrimaryDirectionalEntity();
    void setPrimaryDirectionalEntity(Entity entity);
    void clearPrimaryDirectionalEntity() { this->primaryDirLightID = entt::null; }

    // Hierarchy functions.
    glm::mat4 computeGlobalTransform(Entity entity);
    glm::quat computeGlobalRotation(Entity entity);

    template <typename T>
    bool hierarchyHasComponent(Entity entity)
    {
      if (entity.hasComponent<ParentEntityComponent>())
      {
        auto& parent = entity.getComponent<ParentEntityComponent>().parent;
        if (entity.hasComponent<T>())
          return true;
        else
          return this->hierarchyHasComponent<T>(parent);
      }
      else
        return entity.hasComponent<T>();
    }

    // Copy the components of other into a new scene.
    void copyForRuntime(Scene &other);

    // Clear the scene is a way that doesn't effect 
    // the renderer handle components of the backup scene.
    void clearForRuntime();

    // Get the filepath and registry.
    entt::registry& getRegistry() { return this->sceneECS; }
    std::string& getSaveFilepath() { return this->saveFilepath; }

    // For profiling.
    float getRenderSubmitTime() const { return this->renderSubmitTime; }
  protected:
    entt::entity primaryCameraID;
    entt::entity primaryDirLightID;

    entt::registry sceneECS;
    std::string saveFilepath;

    float renderSubmitTime;

    friend class Entity;
    friend class SceneGraphWindow;
  };
}
