#include "PhysicsEngine/API/ActivationListener.h"

// Project includes.
#include "Core/Logs.h"

// THIS MUST BE THREAD SAFE
namespace Strontium::PhysicsEngine
{
  ActivationListener::ActivationListener()
  { }
  
  ActivationListener::~ActivationListener()
  { }
  
  void 
  ActivationListener::OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
  { }
  
  void 
  ActivationListener::OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
  { }
}