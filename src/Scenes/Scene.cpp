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

  void
  Scene::deleteEntity(Entity entity)
  {
    this->sceneECS.destroy(entity);
  }

  void
  Scene::onUpdate(float dt, Shared<Camera> sceneCamera)
  {
    auto modelAssets = AssetManager<Model>::getManager();

    // Group together the transform and renderable components.
    auto drawables = this->sceneECS.group<TransformComponent>(entt::get<RenderableComponent>);
    for (auto entity : drawables)
    {
      // Draw all the renderables with transforms.
      auto [transform, renderable] = drawables.get<TransformComponent, RenderableComponent>(entity);

      if (renderable)
      {
        // Quick sanity check to make sure the number of materials is the same as
        // the number of submeshes. If not, clear the materials and regenerate them.
        auto& materials = renderable.materials.getStorage();
        auto& submeshes = modelAssets->getAsset(renderable.meshName)->getSubmeshes();

        if (materials.size() != submeshes.size())
        {
          materials.clear();
          for (auto& pair : submeshes)
            renderable.materials.attachMesh(pair.second);
        }

        // Draw the mesh with the associated transform.
        Renderer3D::draw(renderable, renderable, transform, sceneCamera);
      }
    }

    // Draw the environment at the end of the frame.
    Renderer3D::drawEnvironment(sceneCamera);
  }
}
