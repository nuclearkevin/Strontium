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

    void onUpdateEditor(float dt);
    void onUpdateRuntime(float dt);
    void onRenderEditor(Entity selectedEntity);
    void onRenderRuntime();

    Entity getPrimaryCameraEntity();
    void setPrimaryCameraEntity(Entity entity);
    void clearPrimaryCameraEntity() { this->primaryCameraID = entt::null; }

    Entity getPrimaryDirectionalEntity();
    void setPrimaryDirectionalEntity(Entity entity);
    void clearPrimaryDirectionalEntity() { this->primaryDirLightID = entt::null; }

    entt::registry& getRegistry() { return this->sceneECS; }
    std::string& getSaveFilepath() { return this->saveFilepath; }
  protected:
    glm::mat4 computeGlobalTransform(Entity parent);
    void updateAnimations(float dt);

    entt::entity primaryCameraID;
    entt::entity primaryDirLightID;

    entt::registry sceneECS;
    std::string saveFilepath;

    friend class Entity;
    friend class SceneGraphWindow;
  };
}
