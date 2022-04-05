#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Model.h"
#include "Graphics/Textures.h"
#include "Scenes/Scene.h"

// STL includes.
#include <filesystem>

namespace Strontium
{
  namespace AsyncLoading
  {
    // Async load a model.
    void bulkGenerateMaterials();
    void asyncLoadModel(const std::filesystem::path &filepath, const std::string &name,
                        uint entityID, Scene* activeScene);

    // Async load an image.
    void bulkGenerateTextures();
    void loadImageAsync(const std::filesystem::path &filepath,
                        const Texture2DParams &params = Texture2DParams());
  };
}
