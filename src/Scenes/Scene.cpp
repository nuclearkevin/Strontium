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
    // Get all the skyboxes (there should only be one per ECS).
    auto skyboxes = this->sceneECS.view<AmbientComponent>();
    for (auto entity : skyboxes)
    {
      auto skybox = skyboxes.get<AmbientComponent>(entity);
      if (skybox)
      {
        skybox.ambient->bind(MapType::Irradiance, 0);
        skybox.ambient->bind(MapType::Prefilter, 1);
        skybox.ambient->bind(MapType::Integration, 2);
        skybox.ambient->getGamma() = skybox.gamma;
        skybox.ambient->getRoughness() = skybox.roughness;
      }
    }

    // Group together the transform and renderable components.
    auto drawables = this->sceneECS.group<TransformComponent>(entt::get<RenderableComponent>);
    for (auto entity : drawables)
    {
      // Draw all the renderables with transforms.
      auto [transform, renderable] = drawables.get<TransformComponent, RenderableComponent>(entity);

      // Quick sanity check to make sure the number of materials is the same as
      // the number of submeshes. If not, clear the materials and regenerate them.
      auto& materials = renderable.materials.getStorage();
      auto& submeshes = renderable.model->getSubmeshes();

      if (materials.size() == 0)
      {
        for (auto& pair : submeshes)
          renderable.materials.attachMesh(pair.second);
      }

      // Draw the mesh with the associated transform.
      if (renderable)
        Renderer3D::draw(renderable, renderable, transform, sceneCamera);
    }

    for (auto entity : skyboxes)
    {
      // Draw the skybox.
      auto skybox = skyboxes.get<AmbientComponent>(entity);
      if (skybox)
        Renderer3D::draw(skybox, sceneCamera);
    }
  }
}
