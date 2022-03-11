#pragma once

// Project includes.
#include "Core/ApplicationBase.h"

// Jolt includes.
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"

namespace Strontium
{
  class PhysicsActivationListener : public JPH::BodyActivationListener
  {
  public:
	PhysicsActivationListener();
	~PhysicsActivationListener() override;

	void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override;

	void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override;
  private:

  };
}