#include "GuiElements/GuiWindow.h"

namespace SciRenderer
{
  GuiWindow::GuiWindow(bool isOpen)
    : isOpen(isOpen)
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
