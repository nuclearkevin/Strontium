#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Scenes/Scene.h"

namespace SciRenderer
{
  enum class FileLoadTargets
  {
    TargetScene, TargetTexture, TargetModel, TargetEnvironment, TargetNone
  };

  enum class FileSaveTargets
  {
    TargetScene, TargetPrefab, TargetMaterial, TargetNone
  };

  class EditorLayer;

  class GuiWindow
  {
  public:
    GuiWindow(EditorLayer* parentLayer, bool isOpen = true);
    virtual ~GuiWindow();

    virtual void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    virtual void onUpdate(float dt, Shared<Scene> activeScene);
    virtual void onEvent(Event &event);

    bool isOpen;
  protected:
    EditorLayer* parentLayer;
  };
}
