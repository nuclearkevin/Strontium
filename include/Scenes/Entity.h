#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Entity component system include.
#include "entt.hpp"

// Project includes.
#include "Scenes/Scene.h"

namespace SciRenderer
{
  // Entity class, which wraps up the entt ECS functionality to make it easier
  // to use.
  // This is heavily inspired by the work done in Hazel, a 2D game engine by
  // TheCherno.
  // https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Scene/Entity.h
  class Entity
  {
  public:
    Entity();
    Entity(entt::entity entHandle, Scene* scene);
    Entity(const Entity &other) = default;
    ~Entity() = default;

    // Templated functions to make adding component to the engine easier.
    // Check to see if the entity has a component.
    template <typename T>
    bool hasComponent() { return this->parentScene->sceneECS.has<T>(this->entityID); }

    // Add a component. Asserts if the entity already has a component of that type.
    template <typename T, typename ... Args>
    T& addComponent(Args&& ... args)
    {
      assert(!this->hasComponent<T>());
      T& newComponent = this->parentScene->sceneECS.emplace<T>(this->entityID, std::forward<Args>(args)...);
      return newComponent;
    }

    // Remove a component. Asserts if there is no component of the type.
    template<typename T>
    void removeComponent()
    {
      assert(this->hasComponent<T>());
      this->parentScene->sceneECS.remove<T>(this->entityID);
    }

    // Get the component related to this entity. Asserts if there is no component.
    template<typename T>
    T& getComponent()
    {
      // Eventually I'll get around to writing a custom assertion system.
      assert(this->hasComponent<T>());
      return this->parentScene->sceneECS.get<T>(this->entityID);
    }

    // Operator overloading to make using this wrapper easier.
    operator bool() { return this->entityID != entt::null; }
    operator entt::entity() { return this->entityID; }
    operator GLuint() { return (GLuint) this->entityID; }
    bool operator==(const Entity& other) { return this->entityID == other.entityID
                                           && this->parentScene == other.parentScene; }
    bool operator!=(const Entity& other) { return !(*this == other); }

  private:
    entt::entity entityID;
    Scene* parentScene;
  };
}
