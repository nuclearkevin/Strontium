#include "GuiElements/GuiWindow.h"

namespace SciRenderer
{
  GuiWindow::GuiWindow(EditorLayer* parentLayer, bool isOpen)
    : parentLayer(parentLayer)
    , isOpen(isOpen)
  { }

  GuiWindow::~GuiWindow()
  { }

  void
  GuiWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  { }

  void
  GuiWindow::onUpdate(float dt, Shared<Scene> activeScene)
  { }

  void
  GuiWindow::onEvent(Event &event)
  { }
}
