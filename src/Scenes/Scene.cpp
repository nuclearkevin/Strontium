#include "Scenes/Scene.h"

// Project includes.
#include "Core/AssetManager.h"
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

  Entity
  Scene::createEntity(GLuint entityID, const std::string& name)
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
    auto modelAssets = AssetManager<Model>::getManager();

    auto ambient = Renderer3D::getStorage()->currentEnvironment.get();
    ambient->bind(MapType::Irradiance, 0);
    ambient->bind(MapType::Prefilter, 1);
    ambient->bind(MapType::Integration, 2);

    // Group together the transform and renderable components.
    auto drawables = this->sceneECS.group<TransformComponent>(entt::get<RenderableComponent>);
    for (auto entity : drawables)
    {
      // Draw all the renderables with transforms.
      auto [transform, renderable] = drawables.get<TransformComponent, RenderableComponent>(entity);

      // Submit the mesh + material + transform to the deferred renderer queue.
      if (renderable)
      {
        auto& materials = renderable.materials.getStorage();
        auto& submeshes = modelAssets->getAsset(renderable.meshName)->getSubmeshes();

        if (materials.size() != submeshes.size())
        {
          materials.clear();
          for (auto& pair : submeshes)
            renderable.materials.attachMesh(pair.second->getName());
        }

        Renderer3D::submit(renderable, renderable, transform);
      }
    }

    // Draw the environment at the end of the frame.
    Renderer3D::drawEnvironment(sceneCamera);
  }
}
