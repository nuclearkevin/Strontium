#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Assets/AssetManager.h"

#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"

#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Animations.h"
#include "Graphics/ShadingPrimatives.h"

#include "Scenes/Entity.h"

#include "PhysicsEngine/PhysicsFlags.h"

namespace Strontium
{
  // Entity nametag component.
  struct NameComponent
  {
    std::string name;
    std::string description;

    NameComponent(const NameComponent&) = default;

    NameComponent()
      : name("")
      , description("")
    { }

    NameComponent(const std::string &name, const std::string &description)
      : name(name)
      , description(description)
    { }

    friend std::ostream& operator<<(std::ostream& os, const NameComponent &component)
    {
      os << "Name: " << component.name << "\n" << "Description: "
         << component.description;
      return os;
    }

    operator std::string()
    {
      return "Name: " + name + "\n" + "Description: " + description;
    }
  };

  //----------------------------------------------------------------------------
  // Internal components. Cannot be added or removed by a user.
  //----------------------------------------------------------------------------
  // A parent entity component so each child entity knows its parents.
  struct ParentEntityComponent
  {
    Entity parent;

    ParentEntityComponent(const ParentEntityComponent&) = default;

    ParentEntityComponent(Entity parent)
      : parent(parent)
    { }

    ParentEntityComponent() = default;
  };

  // A child entity component so each parent component knows its children.
  struct ChildEntityComponent
  {
    std::vector<Entity> children;

    ChildEntityComponent(const ChildEntityComponent&) = default;

    ChildEntityComponent() = default;
  };

  // Prefab component so each prefab can be iterated over and updated in synch.
  struct PrefabComponent
  {
    std::string prefabID;
    std::string prefabPath;
    bool synch;

    PrefabComponent(const PrefabComponent&) = default;

    PrefabComponent(const std::string &prefabID, const std::string &prefabPath)
      : prefabID(prefabID)
      , synch(false)
      , prefabPath(prefabPath)
    { }
  };
  //----------------------------------------------------------------------------

  // Entity transform component.
  struct TransformComponent
  {
    glm::vec3 translation;
    glm::vec3 rotation; // Euler angles. Pitch = x, yaw = y, roll = z.
    glm::vec3 scale;

    TransformComponent(const TransformComponent&) = default;

    TransformComponent()
      : translation(glm::vec3(0.0f))
      , rotation(glm::vec3(0.0f))
      , scale(glm::vec3(1.0f))
    { }

    TransformComponent(const glm::vec3 &translation, const glm::vec3 &rotation,
                       const glm::vec3 &scale)
      : translation(translation)
      , rotation(rotation)
      , scale(scale)
    { }

    // Cast to glm::mat4 for easy multiplication.
    operator glm::mat4()
    {
      return glm::translate(glm::mat4(1.0f), translation)
             * glm::toMat4(glm::quat(rotation)) * glm::scale(scale);
    }
  };

  // Renderable component. This is the component which is passed to the renderer
  // alongside the transform component.
  struct RenderableComponent
  {
    // Model handle and a collection of materials for the model's submeshes.
    ModelMaterial materials;
    Asset::Handle meshName;

    // An animator and the handle of the current animation.
    Animator animator;
    std::string animationHandle;

    RenderableComponent(const RenderableComponent&) = default;

    RenderableComponent()
      : meshName("")
      , animationHandle("")
    { }

    RenderableComponent(const std::string &meshName)
      : meshName(meshName)
      , animationHandle("")
    { }
  };

  // The camera component contants some of the information required to construct
  // a camera shading component. Transform component also required.
  struct CameraComponent
  {
    Camera entCamera;

    bool visualize;

    CameraComponent(const CameraComponent&) = default;

    CameraComponent()
      : entCamera()
      , visualize(false)
    { }
  };

  // All falloffs are in km.
  struct SkyAtmosphereComponent
  {
    // Bunch all of the atmospheric scattering parameters together.
    glm::vec4 rayleighScat; // Rayleigh scattering base (x, y, z) and height falloff (w).
    glm::vec4 rayleighAbs; // Rayleigh absorption base (x, y, z) and height falloff (w).
    glm::vec4 mieScat; // Mie scattering base (x, y, z) and height falloff (w).
    glm::vec4 mieAbs; // Mie absorption base (x, y, z) and height falloff (w).
    glm::vec4 ozoneAbs; // Ozone absorption base (x, y, z) and scale (w).
    glm::vec3 planetAlbedo; // Planet albedo (x, y, z).
    glm::vec2 planetAtmRadius; // Planet radius (x) and the atmosphere radius (y).

    // Other parameters which won't be sent to the shader.
    // Couple with the primary light for volumetric shadows and godrays.
    bool usePrimaryLight;

    // Internal renderer handle.
    RendererDataHandle handle;

    SkyAtmosphereComponent(const SkyAtmosphereComponent& other)
      : rayleighScat(other.rayleighScat)
      , rayleighAbs(other.rayleighAbs)
      , mieScat(other.mieScat)
      , mieAbs(other.mieAbs)
      , ozoneAbs(other.ozoneAbs)
      , planetAlbedo(other.planetAlbedo)
      , planetAtmRadius(other.planetAtmRadius)
      , usePrimaryLight(other.usePrimaryLight)
    {
      this->handle = Renderer3D::getPassManager().getRenderPass<SkyAtmospherePass>()->requestRendererData();
    }

    SkyAtmosphereComponent operator=(const SkyAtmosphereComponent& other)
    {
      this->rayleighScat = other.rayleighScat;
      this->rayleighAbs = other.rayleighAbs;
      this->mieScat = other.mieScat;
      this->mieAbs = other.mieAbs;
      this->ozoneAbs = other.ozoneAbs;
      this->planetAlbedo = other.planetAlbedo;
      this->planetAtmRadius = other.planetAtmRadius;
      this->usePrimaryLight = other.usePrimaryLight;

      this->handle = Renderer3D::getPassManager().getRenderPass<SkyAtmospherePass>()->requestRendererData();

      return *this;
    }

    SkyAtmosphereComponent(SkyAtmosphereComponent&& other) noexcept
      : rayleighScat(other.rayleighScat)
      , rayleighAbs(other.rayleighAbs)
      , mieScat(other.mieScat)
      , mieAbs(other.mieAbs)
      , ozoneAbs(other.ozoneAbs)
      , planetAlbedo(other.planetAlbedo)
      , planetAtmRadius(other.planetAtmRadius)
      , usePrimaryLight(other.usePrimaryLight)
    {
      this->handle = other.handle;
      other.handle = -1;
    }

    SkyAtmosphereComponent& operator=(SkyAtmosphereComponent&& other) noexcept
    {
      if (this != &other)
      {
        this->rayleighScat = other.rayleighScat;
        this->rayleighAbs = other.rayleighAbs;
        this->mieScat = other.mieScat;
        this->mieAbs = other.mieAbs;
        this->ozoneAbs = other.ozoneAbs;
        this->planetAlbedo = other.planetAlbedo;
        this->planetAtmRadius = other.planetAtmRadius;
        this->usePrimaryLight = other.usePrimaryLight;
        
        this->handle = other.handle;
        other.handle = -1;
      }

      return *this;
    }

    SkyAtmosphereComponent()
      : rayleighScat(5.802f, 13.558f, 33.1f, 8.0f)
      , rayleighAbs(0.0f, 0.0f, 0.0f, 8.0f)
      , mieScat(3.996f, 3.996f, 3.996f, 1.2f)
      , mieAbs(4.4f, 4.4f, 4.4f, 1.2f)
      , ozoneAbs(0.650f, 1.881f, 0.085f, 0.002f)
      , planetAlbedo(0.0f, 0.0f, 0.0f)
      , planetAtmRadius(6.360f, 6.460f) // In MM.
      , usePrimaryLight(false)
      , handle(Renderer3D::getPassManager().getRenderPass<SkyAtmospherePass>()->requestRendererData())
    { }

    ~SkyAtmosphereComponent()
    {
      Renderer3D::getPassManager().getRenderPass<SkyAtmospherePass>()->deleteRendererData(this->handle);
    }

    operator Atmosphere()
    {
      return Atmosphere(rayleighScat, rayleighAbs, mieScat, 
                        mieAbs, ozoneAbs, glm::vec4(planetAlbedo, planetAtmRadius.x), 
                        glm::vec4(0.0f, 0.0f, 0.0f, planetAtmRadius.y), 
                        glm::vec4(0.0f), glm::vec4(0.0f));
    }

    operator RendererDataHandle() { return this->handle; }
  };

  // The dynamic skybox component.
  struct DynamicSkyboxComponent
  {
    float sunSize;
    float intensity;

    DynamicSkyboxComponent(const DynamicSkyboxComponent&) = default;

    DynamicSkyboxComponent()
      : sunSize(1.0f)
      , intensity(1.0f)
    { }
  };

  struct BoxFogVolumeComponent
  {
    float phase;
    float density; // Multiplied by the coefficients.
    float absorption;
    glm::vec3 mieScattering;
    glm::vec3 emission;

    BoxFogVolumeComponent(const BoxFogVolumeComponent&) = default;

    BoxFogVolumeComponent()
      : phase(0.8f)
      , density(0.01f)
      , absorption(1.0f)
      , mieScattering(0.1f)
      , emission(0.0f)
    { }
  };

  struct SphereFogVolumeComponent
  {
    float phase;
    float density; // Multiplied by the coefficients.
    float absorption;
    float radius;
    glm::vec3 mieScattering;
    glm::vec3 emission;

    SphereFogVolumeComponent(const SphereFogVolumeComponent&) = default;

    SphereFogVolumeComponent()
        : phase(0.8f)
        , density(0.01f)
        , absorption(1.0f)
        , radius(1.0f)
        , mieScattering(0.1f)
        , emission(0.0f)
    { }
  };

  // TODO: Finish these.
  // Various light components for rendering the scene.
  struct DirectionalLightComponent
  {
    glm::vec3 colour;
    float intensity;
    glm::vec3 direction;

    bool castShadows;
    float size;

    DirectionalLightComponent(const DirectionalLightComponent&) = default;

    DirectionalLightComponent()
      : colour(1.0f)
      , intensity(1.0f)
      , direction(0.0f, -1.0f, 0.0f)
      , castShadows(false)
      , size(10.f)
    { }

    operator DirectionalLight()
    {
      return DirectionalLight(this->colour, this->intensity, this->direction, this->size);
    }
  };

  struct PointLightComponent
  {
    float radius;
    glm::vec3 colour;
    float intensity;

    bool castShadows;

    PointLightComponent(const PointLightComponent&) = default;

    PointLightComponent()
      : radius(1.0f)
      , colour(1.0f)
      , intensity(1.0f)
      , castShadows(false)
    { }

    operator PointLight()
    {
      return PointLight({ 0.0f, 0.0f, 0.0f, this->radius }, { this->colour, this->intensity });
    }
  };

  struct RectAreaLightComponent
  {
    glm::vec3 colour;
    float intensity;

    float radius;

    bool twoSided;
    bool cull;

    RectAreaLightComponent(const RectAreaLightComponent&) = default;

    RectAreaLightComponent()
      : colour(1.0f)
      , intensity(1.0f)
      , radius(1.0f)
      , twoSided(false)
      , cull(true)
    { }

    operator RectAreaLight()
    {
      return RectAreaLight( { this->colour, this->intensity } );
    }
  };

  struct DynamicSkylightComponent
  {
    float intensity;

    RendererDataHandle handle;

    DynamicSkylightComponent(const DynamicSkylightComponent& other)
      : intensity(other.intensity)
    {
      this->handle = Renderer3D::getPassManager().getRenderPass<DynamicSkyIBLPass>()->requestRendererData();
    }

    DynamicSkylightComponent operator=(const DynamicSkylightComponent& other)
    {
      this->intensity = other.intensity;

      this->handle = Renderer3D::getPassManager().getRenderPass<DynamicSkyIBLPass>()->requestRendererData();

      return *this;
    }

    DynamicSkylightComponent(DynamicSkylightComponent&& other) noexcept
      : intensity(other.intensity)
    {
      this->handle = other.handle;
      other.handle = -1;
    }

    DynamicSkylightComponent& operator=(DynamicSkylightComponent&& other) noexcept
    {
      if (this != &other)
      {
        this->intensity = other.intensity;
        
        this->handle = other.handle;
        other.handle = -1;
      }

      return *this;
    }

    DynamicSkylightComponent()
      : intensity(1.0f)
      , handle(Renderer3D::getPassManager().getRenderPass<DynamicSkyIBLPass>()->requestRendererData())
    { }

    ~DynamicSkylightComponent()
    {
      Renderer3D::getPassManager().getRenderPass<DynamicSkyIBLPass>()->deleteRendererData(handle);
    }
  };

  // Physics components!
  struct SphereColliderComponent
  {
    PhysicsEngine::ColliderTypes type;
    float radius;
    glm::vec3 offset;

    // TODO: Physics material.
    float density;

    bool visualize;

    SphereColliderComponent(const SphereColliderComponent&) = default;

    SphereColliderComponent()
      : type(PhysicsEngine::ColliderTypes::Sphere)
      , radius(1.0f)
      , offset(0.0f)
      , density(1000.0f)
      , visualize(false)
    { }
  };

  struct BoxColliderComponent
  {
    PhysicsEngine::ColliderTypes type;
    glm::vec3 extents;
    glm::vec3 offset;

    // TODO: Physics material.
    float density;

    bool visualize;

    BoxColliderComponent(const BoxColliderComponent&) = default;

    BoxColliderComponent()
      : type(PhysicsEngine::ColliderTypes::Box)
      , extents(0.5f)
      , offset(0.0f)
      , density(1000.0f)
      , visualize(false)
    { }
  };

  struct CylinderColliderComponent
  {
    PhysicsEngine::ColliderTypes type;
    float halfHeight;
    float radius;
    glm::vec3 offset;

    // TODO: Physics material.
    float density;

    bool visualize;

    CylinderColliderComponent(const CylinderColliderComponent&) = default;

    CylinderColliderComponent()
      : type(PhysicsEngine::ColliderTypes::Cylinder)
      , halfHeight(1.0f)
      , radius(1.0f)
      , offset(0.0f)
      , density(1000.0f)
      , visualize(false)
    { }
  };

  struct CapsuleColliderComponent
  {
    PhysicsEngine::ColliderTypes type;
    float halfHeight;
    float radius;
    glm::vec3 offset;

    // TODO: Physics material.
    float density;

    bool visualize;

    CapsuleColliderComponent(const CapsuleColliderComponent&) = default;

    CapsuleColliderComponent()
      : type(PhysicsEngine::ColliderTypes::Capsule)
      , halfHeight(1.0f)
      , radius(1.0f)
      , offset(0.0f)
      , density(1000.0f)
      , visualize(false)
    { }
  };

  struct RigidBody3DComponent
  {
    PhysicsEngine::RigidBodyTypes type;

    float friction;
    float restitution;

    RigidBody3DComponent(const RigidBody3DComponent&) = default;

    RigidBody3DComponent()
      : type(PhysicsEngine::RigidBodyTypes::Static)
      , friction(0.5f)
      , restitution(0.0f)
    { }
  };
}
