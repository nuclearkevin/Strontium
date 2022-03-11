#pragma once

// Project includes.
#include "Core/ApplicationBase.h"

// Jolt includes.
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "PhysicsSystem/API/PhysicsLayers.h"

namespace Strontium
{
  class PhysicsBPLayerInterface final : public JPH::BroadPhaseLayerInterface
  {
  public:
	PhysicsBPLayerInterface()
	{
	  this->layers[PhysicsLayers::NonMoving] = JPH::BroadPhaseLayer(PhysicsLayers::NonMoving);
	  this->layers[PhysicsLayers::Moving] = JPH::BroadPhaseLayer(PhysicsLayers::Moving);
	}

	~PhysicsBPLayerInterface() override 
	{ }

	uint GetNumBroadPhaseLayers() const override { return PhysicsLayers::NumLayers; }

	JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override 
	{ 
	  assert(inLayer < PhysicsLayers::NumLayers);
	  return this->layers[inLayer]; 
	}

    #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
	  switch ((BroadPhaseLayer::Type)inLayer)
	  {
	    case (BroadPhaseLayer::Type)PhysicsLayers::NonMoving: return "Non-Moving";
	    case (BroadPhaseLayer::Type)PhysicsLayers::Moving: return "Moving";
	    default: assert(false); return "Invalid";
	  }
	}
    #endif
  private:
	JPH::BroadPhaseLayer layers[PhysicsLayers::NumLayers];
  };
}