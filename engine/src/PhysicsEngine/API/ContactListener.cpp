#include "PhysicsEngine/API/ContactListener.h"

// Project includes.
#include "Core/Logs.h"

// Jolt includes.
#include "Jolt/Physics/Body/Body.h"

// THIS MUST BE THREAD SAFE
namespace Strontium::PhysicsEngine
{
  ContactListener::ContactListener()
  {
  
  }
  
  ContactListener::~ContactListener()
  {
  
  }
  
  JPH::ValidateResult 
  ContactListener::OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2,
  	                                 const JPH::CollideShapeResult &inCollisionResult)
  {
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
  }
  
  void 
  ContactListener::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2,
  	                              const JPH::ContactManifold &inManifold,
  	                              JPH::ContactSettings &ioSettings)
  { }
  
  void 
  ContactListener::OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2,
  	                                  const JPH::ContactManifold &inManifold,
  	                                  JPH::ContactSettings &ioSettings)
  { }
  
  void 
  ContactListener::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair)
  { }
}