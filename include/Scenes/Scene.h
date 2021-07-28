#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/GraphicsSystem.h"

// Entity component system include.
#include "entt.hpp"

namespace SciRenderer
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
    void render(Shared<Camera> sceneCamera, Entity selectedEntity);

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
