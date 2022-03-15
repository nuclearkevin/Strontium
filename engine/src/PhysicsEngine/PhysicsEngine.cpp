#include "PhysicsEngine/PhysicsEngine.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Logs.h"

#include "PhysicsEngine/API/Callbacks.h"
#include "PhysicsEngine/API/ThreadPool.h" // TODO: Fix this as it currently deadlocks.
#include "PhysicsEngine/API/BPLayerInterface.h"
#include "PhysicsEngine/API/ContactListener.h"
#include "PhysicsEngine/API/ActivationListener.h"

// Jolt physics includes.
#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Core/JobSystem.h"
#include "Jolt/Core/JobSystemThreadPool.h" // Temporary since my own threadpool deadlocks as of right now.

namespace Strontium::PhysicsEngine
{
  constexpr uint maxNumBodies = 65535u;
  constexpr uint maxNumBodyPairs = 65535u;
  constexpr uint maxNumContactConstraints = 65535u;
  constexpr uint numBodyMutexes = 0u;

  constexpr float dtUpdate = 1.0f / 60.0f;

  struct PhysicsInternals
  {
	JPH::JobSystemThreadPool threadPool;
	//ThreadPool threadPool;
	JPH::TempAllocatorImpl allocator;

	BPLayerInterface bpInterface;
	ContactListener cListener;
	ActivationListener aListener;

	JPH::PhysicsSystem physicsSystem;
	JPH::BodyInterface* bodyInterface;

	bool simulating;
	float accumulator;

	robin_hood::unordered_node_map<entt::entity, PhysicsActor> actors;

	PhysicsInternals()
	  : threadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 2)
	  //: threadPool(JPH::cMaxPhysicsBarriers)
	  , allocator(100 * 1024 * 1024)
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
	Logs::log("Initializing physics.");

	// Install callbacks
	JPH::Trace = Callbacks::traceImpl;
	JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = Callbacks::assertFailedImpl;)

	JPH::RegisterTypes();

	internals = new PhysicsInternals();

	internals->physicsSystem.Init(maxNumBodies, 
								  numBodyMutexes, 
								  maxNumBodyPairs, 
								  maxNumContactConstraints, 
								  internals->bpInterface, 
								  Callbacks::broadPhaseCanCollide, 
								  Callbacks::objectsCanCollide);
	internals->physicsSystem.SetBodyActivationListener(&internals->aListener);
	internals->physicsSystem.SetContactListener(&internals->cListener);

	internals->bodyInterface = &internals->physicsSystem.GetBodyInterface();
  }

  // Call when the application quits.
  void 
  shutdown()
  {
	Logs::log("Shutting down physics.");

	// End the simulation before shutting down the physics system.
	if (internals->simulating)
	  onSimulationEnd();

	delete internals;
  }

  // Check to see if an actor exists.
  bool 
  hasActor(Entity entity)
  {
	auto pos = internals->actors.find(entity);
	return pos != internals->actors.end();
  }

  // Add an actor.
  PhysicsActor& 
  addActor(Entity entity)
  {
	assert(!hasActor(entity));

	internals->actors.emplace(entity, PhysicsActor(entity, internals->bodyInterface));
	return internals->actors.at(entity);
  }

  // Get an actor.
  PhysicsActor& 
  getActor(Entity entity)
  {
	assert(hasActor(entity));
	return internals->actors.at(entity);
  }

  // Remove an actor.
  void 
  removeActor(Entity entity)
  {
	assert(hasActor(entity));
	internals->actors.erase(entity);
  }

  // Call when you want to start a new simulation. 
  // Make sure all initial bodies are submitted first.
  void 
  onSimulationBegin()
  {
	assert(("onSimulationBegin() can only be called if the simulation is not currently running. ", !internals->simulating));

	internals->physicsSystem.OptimizeBroadPhase();

	internals->simulating = true;
  }

  // Call when you want to end a simulation.
  void
  onSimulationEnd()
  {
	assert(("onSimulationEnd() can only be called if the simulation is currently running. ", internals->simulating));

	// Cleanup once the physics system is done simulating.
	for (auto& actorPairs : internals->actors)
	{
	  if (!actorPairs.second.isValid())
	    continue;

	  internals->bodyInterface->RemoveBody(JPH::BodyID(actorPairs.second.getBodyID()));
	  internals->bodyInterface->DestroyBody(JPH::BodyID(actorPairs.second.getBodyID()));
	}
	internals->actors.clear();

	internals->simulating = false;
  }

  void 
  onUpdate(float dtFrame)
  {
	if (!internals->simulating)
	  return;

	// TODO: Check for invalid physics objects and remove them.

	// Simulate physics.
	internals->accumulator += dtFrame;
	uint numSteps = static_cast<uint>(glm::floor(internals->accumulator / dtUpdate));

	for (uint i = 0; i < numSteps; ++i)
	{
	  internals->physicsSystem.Update(dtUpdate, 1, 1, &internals->allocator, &internals->threadPool);
	  internals->accumulator -= dtUpdate;
	}
  }
}