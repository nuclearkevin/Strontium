#include "Scenes/Scene.h"

// Project includes.
#include "Scenes/Components.h"
#include "Scenes/Entity.h"

namespace SciRenderer
{
  Scene::Scene()
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

  void
  Scene::deleteEntity(Entity entity)
  {
    this->sceneECS.destroy(entity);
  }

  void
  Scene::onUpdate(float dt, Shared<Camera> sceneCamera)
  {
    // Get the renderer.
    Renderer3D* renderer = Renderer3D::getInstance();

    auto drawables = this->sceneECS.group<TransformComponent>(entt::get<RenderableComponent>);
    for (auto entity : drawables)
    {
      auto [transform, renderable] = drawables.get<TransformComponent, RenderableComponent>(entity);
      if (renderable)
        renderer->draw(renderable, renderable, transform, sceneCamera);
    }
  }
}
