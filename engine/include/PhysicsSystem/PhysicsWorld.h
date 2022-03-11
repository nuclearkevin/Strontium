#pragma once

namespace Strontium::PhysicsWorld
{
  void init();
  void shutdown();

  void onSimulationBegin();
  void onSimulationEnd();

  void onUpdate(float dtFrame);
}