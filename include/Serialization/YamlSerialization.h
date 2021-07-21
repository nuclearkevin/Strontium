#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Scenes/Scene.h"

namespace SciRenderer
{
  namespace YAMLSerialization
  {
    void serializeScene(Shared<Scene> scene, const std::string &filepath,
                        const std::string &name = "Untitled");

    bool deserializeScene(Shared<Scene> scene, const std::string &filepath);
  }
}
