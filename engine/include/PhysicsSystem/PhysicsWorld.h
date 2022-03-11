#pragma once

namespace Strontium::PhysicsWorld
{
  void init();
  void shutdown();

  void onSimulationBegin();
  void onSimulationEnd();

  void updatePhysics(float dtFrame);
}