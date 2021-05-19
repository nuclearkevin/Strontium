#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Scenes/Components.h"

// Entity component system include.
#include "entt.hpp"

namespace SciRenderer
{
  class Scene
  {
  public:
    Scene();
    ~Scene();

  protected:
    entt::registry sceneECS;

  };
}
