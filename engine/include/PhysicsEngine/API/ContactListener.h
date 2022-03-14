#pragma once

// Project includes.
#include "Core/ApplicationBase.h"

// Jolt includes.
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ContactListener.h"

namespace Strontium::PhysicsEngine
{
  class ContactListener : public JPH::ContactListener
  {
  public:
	ContactListener();
	~ContactListener() override;

	JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, 
										  const JPH::CollideShapeResult &inCollisionResult) override;

	void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, 
						const JPH::ContactManifold &inManifold, 
						JPH::ContactSettings &ioSettings) override;

	void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, 
							const JPH::ContactManifold &inManifold, 
							JPH::ContactSettings &ioSettings) override;

	void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override;

	// TODO: Register collision callbacks and all that good stuff.
  private:

  };
}