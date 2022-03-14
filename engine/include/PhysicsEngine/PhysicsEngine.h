#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Scenes/Entity.h"
#include "PhysicsEngine/PhysicsActor.h"

namespace Strontium::PhysicsEngine
{
  void init();
  void shutdown();

  bool hasActor(Entity entity);
  PhysicsActor& addActor(Entity entity);
  PhysicsActor& getActor(Entity entity);
  void removeActor(Entity entity);

  void onSimulationBegin();
  void onSimulationEnd();

  void onUpdate(float dtFrame);
}