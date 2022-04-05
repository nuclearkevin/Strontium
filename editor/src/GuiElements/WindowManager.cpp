#include "GuiElements/WindowManager.h"

namespace Strontium
{
  void 
  WindowManager::onImGuiRender(Shared<Scene> activeScene)
  {
	for (auto& [id, window] : this->managedWindows)
      if (window->isOpen)
	    window->onImGuiRender(window->isOpen, activeScene);
  }

  void 
  WindowManager::onUpdate(float dt, Shared<Scene> activeScene)
  {
	for (auto& [id, window] : this->managedWindows)
	  window->onUpdate(dt, activeScene);
  }

  void 
  WindowManager::onEvent(Event &event)
  {
	for (auto& [id, window] : this->managedWindows)
	  window->onEvent(event);
  }
}