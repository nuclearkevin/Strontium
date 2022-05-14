#include "PhysicsEngine/PhysicsActor.h"

// Project includes.
#include "Core/Logs.h"
#include "Scenes/Components.h"
#include "PhysicsEngine/PhysicsUtils.h"
#include "PhysicsEngine/API/Layers.h"

// GLM includes.
#include "glm/gtx/matrix_decompose.hpp"

// Jolt includes.
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/Body.h"

namespace Strontium::PhysicsEngine
{
  PhysicsActor 
  PhysicsActor::createActor(Entity owningEntity, JPH::BodyInterface* owningInterface)
  {
    PhysicsActor newActor;
    
    assert(("Invalid entity.", static_cast<bool>(owningEntity)));
    assert(("PhysicsActor requires the owning entity to have a transform and rigidbody component.", 
            !(!owningEntity.hasComponent<RigidBody3DComponent>() || !owningEntity.hasComponent<TransformComponent>())));
    assert(("JPH::BodyInterface* was null.", owningInterface));

    Entity parent = owningEntity.hasComponent<ParentEntityComponent>() ? owningEntity.getComponent<ParentEntityComponent>().parent : Entity();
    if (parent)
    {
      if (static_cast<Scene*>(parent)->hierarchyHasComponent<RigidBody3DComponent>(parent))
      {
        Logs::log("Warning: Actors cannot have parents with RigidBody components.");
        newActor.valid = false;
        return newActor;
      }
    }

    JPH::ShapeRefC shapeRef;
    glm::vec3 offset(0.0f);

    //----------------------------------------------------------------------------
    // Sphere shape.
    //----------------------------------------------------------------------------
    if (owningEntity.hasComponent<SphereColliderComponent>() && !shapeRef)
    {
      auto& collider = owningEntity.getComponent<SphereColliderComponent>();

      // Create the Jolt sphere shape.
      JPH::SphereShapeSettings shapeSettings(collider.radius);
      shapeSettings.mDensity = collider.density;

      // Register the sphere shape and error check.
      JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
      if (!result.HasError())
      {
        shapeRef = result.Get();
        newActor.cType = collider.type;
      }
      else
        Logs::log("Warning: Failed to create a box shape with the message: " + result.GetError());

      offset = collider.offset;
    }

    //----------------------------------------------------------------------------
    // Box shape.
    //----------------------------------------------------------------------------
    if (owningEntity.hasComponent<BoxColliderComponent>() && !shapeRef)
    {
      auto& collider = owningEntity.getComponent<BoxColliderComponent>();

      // Create the Jolt box shape.
      JPH::BoxShapeSettings shapeSettings(PhysicsUtils::convertGLMToJolt(collider.extents));
      shapeSettings.mDensity = collider.density;

      // Register the box shape and error check.
      JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
      if (!result.HasError())
      {
        shapeRef = result.Get();
        newActor.cType = collider.type;
      }
      else
        Logs::log("Warning: Failed to create a box shape with the message: " + result.GetError());

      offset = collider.offset;
    }

    //----------------------------------------------------------------------------
    // Cylinder shape.
    //----------------------------------------------------------------------------
    if (owningEntity.hasComponent<CylinderColliderComponent>() && !shapeRef)
    {
      auto& collider = owningEntity.getComponent<CylinderColliderComponent>();

      JPH::CylinderShapeSettings shapeSettings(collider.halfHeight, collider.radius);
      shapeSettings.mDensity = collider.density;

      // Register the cylinder shape and error check.
      JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
      if (!result.HasError())
      {
        shapeRef = result.Get();
        newActor.cType = collider.type;
      }
      else
        Logs::log("Warning: Failed to create a cylinder shape with the message: " + result.GetError());

      offset = collider.offset;
    }

    //----------------------------------------------------------------------------
    // Capsule shape.
    //----------------------------------------------------------------------------
    if (owningEntity.hasComponent<CapsuleColliderComponent>() && !shapeRef)
    {
      auto& collider = owningEntity.getComponent<CapsuleColliderComponent>();

      JPH::CapsuleShapeSettings shapeSettings(collider.halfHeight, collider.radius);
      shapeSettings.mDensity = collider.density;

      // Register the cylinder shape and error check.
      JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
      if (!result.HasError())
      {
        shapeRef = result.Get();
        newActor.cType = collider.type;
      }
      else
        Logs::log("Warning: Failed to create a capsule shape with the message: " + result.GetError());

      offset = collider.offset;
    }

    if (!shapeRef)
    {
      newActor.valid = false;
      return PhysicsActor();
    }

    // Grab the transform and rigid body components.
    auto& transform = owningEntity.getComponent<TransformComponent>();
    auto& rigidBody = owningEntity.getComponent<RigidBody3DComponent>();

    auto localTransformMatrix = static_cast<glm::mat4>(transform);
    auto localTransformNoScale = glm::translate(transform.translation) * glm::toMat4(glm::quat(transform.rotation));
    auto globalTransform = static_cast<Scene*>(owningEntity)->computeGlobalTransform(owningEntity);
    auto globalTransformNoLocalScale = globalTransform * glm::inverse(localTransformMatrix);
    globalTransformNoLocalScale = globalTransformNoLocalScale * localTransformNoScale;

    auto rotation = static_cast<Scene*>(owningEntity)->computeGlobalRotation(owningEntity);

    newActor.previousPosition = glm::vec3(globalTransformNoLocalScale * glm::vec4(offset, 1.0f));
    newActor.previousOrientation = glm::quat(rotation);

    auto pos = PhysicsUtils::convertGLMToJolt(newActor.previousPosition);
    auto rot = PhysicsUtils::convertGLMToJolt(newActor.previousOrientation);
    newActor.rbType = rigidBody.type;

    //----------------------------------------------------------------------------
    // Set the body.
    //----------------------------------------------------------------------------
    switch (rigidBody.type)
    {
      case RigidBodyTypes::Static:
      {
        JPH::BodyCreationSettings bodySettings(shapeRef, pos, rot, JPH::EMotionType::Static, Layers::NonMoving);
        JPH::Body* body = owningInterface->CreateBody(bodySettings);
        newActor.joltBodyID = body->GetID().GetIndexAndSequenceNumber();
        owningInterface->AddBody(body->GetID(), JPH::EActivation::DontActivate);

        if (!body)
        {
          Logs::log("Error: Body limit exceeded, failed to create a body.");
          newActor.valid = false;
          return PhysicsActor();
        }
        break;
      }

      case RigidBodyTypes::Kinematic:
      {
        JPH::BodyCreationSettings bodySettings(shapeRef, pos, rot, JPH::EMotionType::Kinematic, Layers::Moving);
        JPH::Body* body = owningInterface->CreateBody(bodySettings);
        newActor.joltBodyID = body->GetID().GetIndexAndSequenceNumber();
        owningInterface->AddBody(body->GetID(), JPH::EActivation::Activate);

        if (!body)
        {
          Logs::log("Error: Body limit exceeded, failed to create a body.");
          newActor.valid = false;
          return PhysicsActor();
        }
        break;
      }

      case RigidBodyTypes::Dynamic:
      {
        JPH::BodyCreationSettings bodySettings(shapeRef, pos, rot, JPH::EMotionType::Dynamic, Layers::Moving);
        JPH::Body* body = owningInterface->CreateBody(bodySettings);
        newActor.joltBodyID = body->GetID().GetIndexAndSequenceNumber();
        owningInterface->AddBody(body->GetID(), JPH::EActivation::Activate);

        if (!body)
        {
          Logs::log("Error: Body limit exceeded, failed to create a body.");
          newActor.valid = false;
          return PhysicsActor();
        }
        break;
      }
    }
    
    newActor.valid = true;
    newActor.owningInterface = owningInterface;
    return newActor;
  }

  PhysicsActor::PhysicsActor()
    : rbType(RigidBodyTypes::Static)
    , cType(ColliderTypes::Sphere)
    , owningInterface(nullptr)
    , joltBodyID()
    , valid(false)
    , previousPosition(0.0f)
    , lerpedPosition(0.0f)
    , previousOrientation(0.0f, 0.0f, 0.0f, 1.0f)
    , slerpedOrientation(0.0f, 0.0f, 0.0f, 1.0f)
  { }

  void 
  PhysicsActor::updatePreviousState()
  {
    this->previousPosition = this->getPosition();
    this->previousOrientation = this->getRotation();
  }

  void 
  PhysicsActor::updateCurrentState(float stateAlpha)
  {
    this->lerpedPosition = glm::mix(this->previousPosition, this->getPosition(), stateAlpha);
    this->slerpedOrientation = glm::mix(this->previousOrientation, this->getRotation(), stateAlpha);
  }

  glm::vec3 
  PhysicsActor::getPosition() const
  {
    assert(("Invalid actor.", this->valid));

    auto pos = this->owningInterface->GetPosition(JPH::BodyID(this->joltBodyID));
  	return PhysicsUtils::convertJoltToGLM(pos);
  }
  
  glm::quat 
  PhysicsActor::getRotation() const
  {
    assert(("Invalid actor.", this->valid));

    auto rot = this->owningInterface->GetRotation(JPH::BodyID(this->joltBodyID));
    return PhysicsUtils::convertJoltToGLM(rot);
  }

  UpdatedTransformData PhysicsActor::getUpdatedTransformData() const
  {
    return UpdatedTransformData(this->lerpedPosition, this->slerpedOrientation);
  }
  
  glm::vec3 
  PhysicsActor::getLinearVelocity() const
  {
    assert(("Invalid actor.", this->valid));

  	auto vel = this->owningInterface->GetLinearVelocity(JPH::BodyID(this->joltBodyID));
    return PhysicsUtils::convertJoltToGLM(vel);
  }
  
  glm::vec3 
  PhysicsActor::getAngularVelocity() const
  {
    assert(("Invalid actor.", this->valid));

  	auto vel = this->owningInterface->GetAngularVelocity(JPH::BodyID(this->joltBodyID));
    return PhysicsUtils::convertJoltToGLM(vel);
  }
  
  float 
  PhysicsActor::getDensity() const
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to acquire the density of a non-dynamic rigid body.");
      return 0.0f;
    }

    auto shp = this->owningInterface->GetShape(JPH::BodyID(this->joltBodyID));
    switch (this->cType)
    {
      case ColliderTypes::Sphere:
      {
        return static_cast<const JPH::SphereShape*>(shp.GetPtr())->GetDensity();
      }

      case ColliderTypes::Box:
      {
        return static_cast<const JPH::BoxShape*>(shp.GetPtr())->GetDensity();
      }

      case ColliderTypes::Cylinder:
      {
        return static_cast<const JPH::CylinderShape*>(shp.GetPtr())->GetDensity();
      }

      case ColliderTypes::Capsule:
      {
        return static_cast<const JPH::CapsuleShape*>(shp.GetPtr())->GetDensity();
      }

      default: return 0.0f;
    }
  }
  
  float 
  PhysicsActor::getRestitution() const
  {
    assert(("Invalid actor.", this->valid));

    auto res = this->owningInterface->GetRestitution(JPH::BodyID(this->joltBodyID));
  	return res;
  }
  
  float 
  PhysicsActor::getFriction() const
  {
    assert(("Invalid actor.", this->valid));

    auto fri = this->owningInterface->GetFriction(JPH::BodyID(this->joltBodyID));
  	return fri;
  }
  
  void 
  PhysicsActor::setPosition(const glm::vec3 &position)
  {
    assert(("Invalid actor.", this->valid));

    auto activate = this->rbType == RigidBodyTypes::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;
    
    this->owningInterface->SetPosition(JPH::BodyID(this->joltBodyID), 
                                       PhysicsUtils::convertGLMToJolt(position),
                                       activate);
  }
  
  void 
  PhysicsActor::setRotation(const glm::quat &rotation)
  {
    assert(("Invalid actor.", this->valid));

    auto activate = this->rbType == RigidBodyTypes::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;
    
    this->owningInterface->SetRotation(JPH::BodyID(this->joltBodyID), 
                                       PhysicsUtils::convertGLMToJolt(rotation),
                                       activate);
  }
  
  void 
  PhysicsActor::setLinearVelocity(const glm::vec3 &velocity)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType == RigidBodyTypes::Static)
    {
      Logs::log("Warning, attempting to set the linear velocity of a static rigid body.");
      return;
    }

    this->owningInterface->SetLinearVelocity(JPH::BodyID(this->joltBodyID), 
                                             PhysicsUtils::convertGLMToJolt(velocity));
  }
  
  void 
  PhysicsActor::setAngularVelocity(const glm::vec3& velocity)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType == RigidBodyTypes::Static)
    {
      Logs::log("Warning, attempting to set the angular velocity of a static rigid body.");
      return;
    }

    this->owningInterface->SetAngularVelocity(JPH::BodyID(this->joltBodyID), 
                                              PhysicsUtils::convertGLMToJolt(velocity));
  }
  
  void 
  PhysicsActor::setDensity(float density)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to set the density of a non-dynamic rigid body.");
      return;
    }

    auto activate = this->rbType == RigidBodyTypes::Static ? JPH::EActivation::DontActivate 
                    : JPH::EActivation::Activate;

    auto shp = this->owningInterface->GetShape(JPH::BodyID(this->joltBodyID));
    switch (this->cType)
    {
      case ColliderTypes::Sphere:
      {
        auto sphere = static_cast<const JPH::SphereShape*>(shp.GetPtr());

        // This feels wrong. Creating a completely new shape when you want to change the density...
        // Create the Jolt sphere shape.
        JPH::SphereShapeSettings shapeSettings(sphere->GetRadius());
        shapeSettings.mDensity = sphere->GetDensity();
        
        // Register the new sphere shape and error check.
        JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
        if (!result.HasError())
        {
          auto shapeRef = result.Get();
          this->owningInterface->SetShape(JPH::BodyID(this->joltBodyID), shapeRef, true, activate);
        }
        else
          Logs::log("Failed to recreate a sphere shape with the error: " + result.GetError());
        return;
      }

      case ColliderTypes::Box:
      {
        auto box = static_cast<const JPH::BoxShape*>(shp.GetPtr());

        // This feels wrong. Creating a completely new shape when you want to change the density...
        // Create the Jolt box shape.
        JPH::BoxShapeSettings shapeSettings(box->GetHalfExtent());
        shapeSettings.mDensity = box->GetDensity();

        // Register the new sphere shape and error check.
        JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
        if (!result.HasError())
        {
          auto shapeRef = result.Get();
          this->owningInterface->SetShape(JPH::BodyID(this->joltBodyID), shapeRef, true, activate);
        }
        else
          Logs::log("Failed to recreate a box shape with the error: " + result.GetError());
        return;
      }

      default: return;
    }
  }
  
  void 
  PhysicsActor::setRestitution(float restitution)
  {
    assert(("Invalid actor.", this->valid));

    this->owningInterface->SetRestitution(JPH::BodyID(this->joltBodyID), restitution);
  }
  
  void
  PhysicsActor::setFriction(float friction)
  {
    assert(("Invalid actor.", this->valid));

    this->owningInterface->SetFriction(JPH::BodyID(this->joltBodyID), friction);
  }
  
  void
  PhysicsActor::addLinearVelocity(const glm::vec3& velocity)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType == RigidBodyTypes::Static)
    {
      Logs::log("Warning, attempting to add to the linear velocity of a static rigid body.");
      return;
    }

    this->owningInterface->AddLinearVelocity(JPH::BodyID(this->joltBodyID), 
                                             PhysicsUtils::convertGLMToJolt(velocity));
  }
  
  void 
  PhysicsActor::addForce(const glm::vec3& force)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to add a force to a non-dynamic rigid body.");
      return;
    }

    this->owningInterface->AddForce(JPH::BodyID(this->joltBodyID), 
                                    PhysicsUtils::convertGLMToJolt(force));
  }
  
  void 
  PhysicsActor::addForce(const glm::vec3& force, const glm::vec3& point)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to add a force to a non-dynamic rigid body.");
      return;
    }

    this->owningInterface->AddForce(JPH::BodyID(this->joltBodyID), 
                                    PhysicsUtils::convertGLMToJolt(force),
                                    PhysicsUtils::convertGLMToJolt(point));
  }
  
  void 
  PhysicsActor::addImpulse(const glm::vec3& impulse)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to add an impulse to a non-dynamic rigid body.");
      return;
    }

    this->owningInterface->AddImpulse(JPH::BodyID(this->joltBodyID), 
                                      PhysicsUtils::convertGLMToJolt(impulse));
  }
  
  void 
  PhysicsActor::addImpulse(const glm::vec3& impulse, const glm::vec3& point)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to add an impulse to a non-dynamic rigid body.");
      return;
    }

    this->owningInterface->AddImpulse(JPH::BodyID(this->joltBodyID), 
                                      PhysicsUtils::convertGLMToJolt(impulse),
                                      PhysicsUtils::convertGLMToJolt(point));
  }
  
  void 
  PhysicsActor::addTorque(const glm::vec3& torque)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to add torque to a non-dynamic rigid body.");
      return;
    }

    this->owningInterface->AddTorque(JPH::BodyID(this->joltBodyID), 
                                     PhysicsUtils::convertGLMToJolt(torque));
  }
  
  void 
  PhysicsActor::addAngularImpulse(const glm::vec3& impulse)
  {
    assert(("Invalid actor.", this->valid));

    if (this->rbType != RigidBodyTypes::Dynamic)
    {
      Logs::log("Warning, attempting to add angular impulse to a non-dynamic rigid body.");
      return;
    }

    this->owningInterface->AddAngularImpulse(JPH::BodyID(this->joltBodyID), 
                                             PhysicsUtils::convertGLMToJolt(impulse));
  }
}