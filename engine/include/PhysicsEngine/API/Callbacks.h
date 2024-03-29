#pragma once

// Project includes.
#include "Core/Logs.h"
#include "PhysicsEngine/API/Layers.h"

// Jolt includes.
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

namespace Strontium::PhysicsEngine::Callbacks
{
  // Callback for traces.
  static void traceImpl(const char *inFMT, ...)
  { 
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
     
    // Print to the TTY
    Logs::log(buffer);
  }

  // Callback for asserts.
  static bool assertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
  {
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
    
    // Breakpoint
    return true;
  };

  // Callback to check if two objects can collide.
  static bool objectsCanCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2)
  {
	switch (inObject1)
	{
	case Layers::NonMoving: return inObject2 == Layers::Moving;
	case Layers::Moving: return true;
	default: return false;
	}
  }

  // Callback to see if two broadphase layers can collide.
  static bool broadPhaseCanCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2)
  {
	switch (inLayer1)
	{
	  case Layers::NonMoving: return inLayer2 == JPH::BroadPhaseLayer(Layers::Moving);
	  case Layers::Moving: return true;
	  default: return false;
	}
  }
}