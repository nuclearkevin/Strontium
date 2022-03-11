#include "PhysicsSystem/API/PhysicsContactListener.h"

// Project includes.
#include "Core/Logs.h"

// Jolt includes.
#include "Jolt/Physics/Body/Body.h"

// THIS MUST BE THREAD SAFE
namespace Strontium
{
  PhysicsContactListener::PhysicsContactListener()
  {
  
  }
  
  PhysicsContactListener::~PhysicsContactListener()
  {
  
  }
  
  JPH::ValidateResult 
  PhysicsContactListener::OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2,
  	                                        const JPH::CollideShapeResult &inCollisionResult)
  {
    Logs::log("Contact valiation callback was made.");
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
  }
  
  void PhysicsContactListener::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2,
  	                                          const JPH::ContactManifold &inManifold,
  	                                          JPH::ContactSettings &ioSettings)
  {
    Logs::log("Contact between two bodies has been adde .");
  }
  
  void PhysicsContactListener::OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2,
  	                                              const JPH::ContactManifold &inManifold,
  	                                              JPH::ContactSettings &ioSettings)
  {
    Logs::log("Contact between two bodies has persisted.");
  }
  
  void PhysicsContactListener::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair)
  {
    Logs::log("Contact between two bodies has been removed.");
  }
}