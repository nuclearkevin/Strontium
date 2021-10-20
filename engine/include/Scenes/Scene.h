#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/ShadingPrimatives.h"

// Entity component system include.
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

    entt::registry& getRegistry() { return this->sceneECS; }
    std::string& getSaveFilepath() { return this->saveFilepath; }
  protected:
    glm::mat4 computeGlobalTransform(Entity parent);

    void updateAnimations(float dt);

    entt::registry sceneECS;

    std::string saveFilepath;

    friend class Entity;
    friend class SceneGraphWindow;
  };
}
