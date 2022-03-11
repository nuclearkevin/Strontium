#include "PhysicsSystem/PhysicsWorld.h"

// Project includes.
#include "Core/ApplicationBase.h"

#include "PhysicsSystem/API/PhysicsCallbacks.h"
#include "PhysicsSystem/API/PhysicsThreadPool.h"
#include "PhysicsSystem/API/PhysicsBPLayerInterface.h"
#include "PhysicsSystem/API/PhysicsContactListener.h"
#include "PhysicsSystem/API/PhysicsActivationListener.h"

// Jolt physics includes.
#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/PhysicsSystem.h"

namespace Strontium::PhysicsWorld
{
  constexpr uint maxNumBodies = 65535u;
  constexpr uint maxNumBodyPairs = 65535u;
  constexpr uint maxNumContactConstraints = 65535u;
  constexpr uint numBodyMutexes = 0u;

  constexpr float dtUpdate = 1.0f / 60.0f;

  struct PhysicsInternals
  {
	PhysicsThreadPool threadPool;
	JPH::TempAllocatorImpl allocator;

	PhysicsBPLayerInterface bpInterface;
	PhysicsContactListener cListener;
	PhysicsActivationListener aListener;

	JPH::PhysicsSystem physicsSystem;
	JPH::BodyInterface* bodyInterface;

	bool simulating;
	float accumulator;

	PhysicsInternals()
	  : threadPool(JPH::cMaxPhysicsBarriers)
	  , allocator(10 * 1024 * 1024)
	  , bpInterface()
	  , cListener()
	  , aListener()
	  , bodyInterface(nullptr)
	  , simulating(false)
	  , accumulator(0.0f)
	{ }
  };

  static PhysicsInternals* internals = nullptr;

  // Call when the application starts.
  void 
  init()
  {
	// Install callbacks
	JPH::Trace = PhysicsCallbacks::traceImpl;
	JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = PhysicsCallbacks::assertFailedImpl;)

	JPH::RegisterTypes();

	internals = new PhysicsInternals();

	internals->physicsSystem.Init(maxNumBodies, 
								  numBodyMutexes, 
								  maxNumBodyPairs, 
								  maxNumContactConstraints, 
								  internals->bpInterface, 
								  PhysicsCallbacks::broadPhaseCanCollide, 
								  PhysicsCallbacks::objectsCanCollide);
	internals->physicsSystem.SetBodyActivationListener(&internals->aListener);
	internals->physicsSystem.SetContactListener(&internals->cListener);

	internals->bodyInterface = &internals->physicsSystem.GetBodyInterface();
  }

  // Call when you want to start a new simulation. 
  // Make sure all initial bodies are submitted first.
  void 
  onSimulationBegin()
  {
	assert(("onSimulationBegin() can only be called if the simulation is not currently running. ", !internals->simulating));

	// TODO: Loop through physics actors and attach bodies.

	internals->simulating = true;
  }

  // Call when you want to end a simulation.
  void
  onSimulationEnd()
  {
	assert(("onSimulationEnd() can only be called if the simulation is currently running. ", internals->simulating));

	// TODO: Loop through physics actors and unattach bodies.

	internals->simulating = false;
  }

  // Call when the application quits.
  void 
  shutdown()
  {
	// End the simulation before shutting down the physics system.
	if (internals->simulating)
	  onSimulationEnd();

	delete internals;
  }

  void 
  onUpdate(float dtFrame)
  {
	if (!internals->simulating)
	  return;

	internals->accumulator += dtFrame;
	uint numSteps = static_cast<uint>(glm::floor(internals->accumulator / dtUpdate));

	for (uint i = 0; i < numSteps; ++i)
	{
	  internals->physicsSystem.Update(dtUpdate, 1, 1, &internals->allocator, &internals->threadPool);
	  internals->accumulator -= dtUpdate;
	}
  }
}