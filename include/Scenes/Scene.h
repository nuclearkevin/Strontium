#pragma once

// Macro include file.
#include "StrontiumPCH.h"

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
    Entity createEntity(GLuint entityID, const std::string& name = "New Entity");
    void recurseDeleteEntity(Entity entity);
    void deleteEntity(Entity entity);

    void onUpdate(float dt);
    void render(Entity selectedEntity);

    entt::registry& getRegistry() { return this->sceneECS; }
    std::string& getSaveFilepath() { return this->saveFilepath; }
  protected:
    glm::mat4 computeGlobalTransform(Entity parent);

    entt::registry sceneECS;

    std::string saveFilepath;

    friend class Entity;
    friend class SceneGraphWindow;
  };
}
