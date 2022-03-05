#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Assets/AssetManager.h"
#include "Scenes/Scene.h"

namespace Strontium
{
  class Shader;

  namespace YAMLSerialization
  {
    void serializeScene(Shared<Scene> scene, const std::string &filepath,
                        const std::string &name = "Untitled");
    void serializeMaterial(const AssetHandle &materialHandle,
                           const std::string &filepath);
    void serializePrefab(Entity prefab, const std::string &filepath, 
                         Shared<Scene> scene, const std::string &name = "Untitled Prefab");

    bool deserializeScene(Shared<Scene> scene, const std::string &filepath);
    bool deserializeMaterial(const std::string &filepath, AssetHandle &handle, bool override = false);
    bool deserializePrefab(Shared<Scene> scene, const std::string &filepath);
  }
}
