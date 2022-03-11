#include "PhysicsSystem/API/PhysicsActivationListener.h"

// Project includes.
#include "Core/Logs.h"

// THIS MUST BE THREAD SAFE
namespace Strontium
{
  PhysicsActivationListener::PhysicsActivationListener()
  {

  }
  
  PhysicsActivationListener::~PhysicsActivationListener()
  {
  
  }
  
  void 
  PhysicsActivationListener::OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
  {
	Logs::log("A body was activated.");
  }
  
  void 
  PhysicsActivationListener::OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
  {
    Logs::log("A body was deactivated.");
  }
}