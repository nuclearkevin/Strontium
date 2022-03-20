#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Scenes/Entity.h"
#include "PhysicsEngine/PhysicsFlags.h"

namespace JPH
{
  class BodyInterface;
  class Body;
}

namespace Strontium::PhysicsEngine
{
  // This is the interface between the JPH::Body and JPH::Shape classes and the rest of the engine.
  // This class assumes that it will be threadsafe (i.e, a locking JPH::BodyInterface is passed as a parameter).
  class PhysicsActor
  {
  public:
	// Create a physics actor given an Entity and a JPH::BodyInterface*.
	static PhysicsActor createActor(Entity owningEntity, JPH::BodyInterface* owningInterface);

	~PhysicsActor() = default;

	// Check if the actor is valid before attempting to modify the actor's state.
	bool isValid() const { return this->valid; }

	// Get the internal Jolt body ID.
	uint getBodyID() const { return this->joltBodyID; }

	// Get the position and orientation of this actor.
	glm::vec3 getPosition() const;
	glm::quat getRotation() const;

	// Get the updated transform data for this actor.
	UpdatedTransformData getUpdatedTransformData() const;

	// Get the velocity of this actor.
	glm::vec3 getLinearVelocity() const;
	glm::vec3 getAngularVelocity() const;

	// Get the physics properties of this actor.
	float getDensity() const;
	float getRestitution() const;
	float getFriction() const;

	// Reposition this actor.
	void setPosition(const glm::vec3 &position);
	void setRotation(const glm::quat &rotation);

	// Update the physics properties of this actor.
	void setDensity(float density);
	void setRestitution(float restitution);
	void setFriction(float friction);

	// Update dynamic and/or kinematic objects.
	void setLinearVelocity(const glm::vec3& velocity);
	void setAngularVelocity(const glm::vec3& velocity);

	void addLinearVelocity(const glm::vec3& velocity);

	void addForce(const glm::vec3 &force);
	void addForce(const glm::vec3 &force, const glm::vec3 &point);

	void addImpulse(const glm::vec3 &impulse);
	void addImpulse(const glm::vec3 &impulse, const glm::vec3 &point);

	void addTorque(const glm::vec3 &torque);
	void addAngularImpulse(const glm::vec3 &impulse);
  private:
	// Construct an invalid actor.
	PhysicsActor(); 

	RigidBodyTypes rbType;
	ColliderTypes cType;

	uint joltBodyID;
	JPH::BodyInterface* owningInterface;

	bool valid;
  };
}