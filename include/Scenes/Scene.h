#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/GraphicsSystem.h"
#include "Scenes/Components.h"

// Entity component system include.
#include "entt.hpp"

namespace SciRenderer
{
  class Entity;

  class Scene
  {
  public:
    Scene();
    ~Scene();

    Entity createEntity(const std::string& name = "New Entity");
    Entity createEntity(GLuint entityID, const std::string& name = "New Entity");
    void deleteEntity(Entity entity);

    void onUpdate(float dt, Shared<Camera> sceneCamera);

    entt::registry& getRegistry() { return this->sceneECS; }
  protected:
    entt::registry sceneECS;

    friend class Entity;
    friend class SceneGraphWindow;
  };
}
