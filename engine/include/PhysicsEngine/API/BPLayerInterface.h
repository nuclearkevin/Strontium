#pragma once

// Project includes.
#include "Core/ApplicationBase.h"

// Jolt includes.
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "PhysicsEngine/API/Layers.h"

namespace Strontium::PhysicsEngine
{
  class BPLayerInterface final : public JPH::BroadPhaseLayerInterface
  {
  public:
	BPLayerInterface()
	{
	  this->layers[Layers::NonMoving] = JPH::BroadPhaseLayer(Layers::NonMoving);
	  this->layers[Layers::Moving] = JPH::BroadPhaseLayer(Layers::Moving);
	}

	~BPLayerInterface() override 
	{ }

	uint GetNumBroadPhaseLayers() const override { return Layers::NumLayers; }

	JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override 
	{ 
	  assert(inLayer < Layers::NumLayers);
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
	JPH::BroadPhaseLayer layers[Layers::NumLayers];
  };
}