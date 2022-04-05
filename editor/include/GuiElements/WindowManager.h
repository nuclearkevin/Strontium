#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "GuiElements/GuiWindow.h"

// STL includes.
#include <typeinfo>

namespace Strontium
{
  class WindowManager
  {
  public:
	WindowManager() = default;
	~WindowManager() = default;

	void onImGuiRender(Shared<Scene> activeScene);
	void onUpdate(float dt, Shared<Scene> activeScene);
	void onEvent(Event &event);

	template <typename T, typename ... Args>
	T* insertWindow(Args ... args)
	{
	  static_assert(std::is_base_of<GuiWindow, T>::value, "Class must derive from GUIWindow.");

      auto typeHash = typeid(T).hash_code();
	  assert((this->managedWindows.find(typeHash) != this->managedWindows.end(), " Cannot have multiples of the same window."));

	  this->managedWindows.emplace(typeHash, createUnique<T>(std::forward<Args>(args)...));
	  return static_cast<T*>(this->managedWindows.at(typeid(T).hash_code()).get());
	}

	template <typename T>
	T* getWindow()
	{
	  static_assert(std::is_base_of<GuiWindow, T>::value, "Class must derive from GUIWindow.");

	  return static_cast<T*>(this->managedWindows.at(typeid(T).hash_code()).get());
	}
  private:
	robin_hood::unordered_flat_map<std::size_t, Unique<GuiWindow>> managedWindows;
  };
}