#include "Physics/PhysicsWorld.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Physics/PhysicsThreadPool.h"

// Jolt physics includes.
#include <Jolt.h>
#include <RegisterTypes.h>
#include <Core/TempAllocator.h>
#include <Physics/PhysicsSettings.h>
#include <Physics/PhysicsSystem.h>

namespace Strontium::PhysicsWorld
{
  struct PhysicsInternals
  {
	PhysicsThreadPool pool;
	JPH::TempAllocatorImpl alloc;

	PhysicsInternals(uint numBarriers, uint memSize)
	  : pool(numBarriers)
	  , alloc(memSize)
	{ }
  };

  static PhysicsInternals* internals = nullptr;

  void init()
  {
	internals = new PhysicsInternals(JPH::cMaxPhysicsBarriers, 10 * 1024 * 1024);
  }

  void 
  shutdown()
  {
	delete internals;
  }
}