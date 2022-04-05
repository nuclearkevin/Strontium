#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Window.h"
#include "Assets/AssetManager.h"
#include "Layers/Layers.h"
#include "Layers/ImGuiLayer.h"
#include "Graphics/Renderer.h"

int main(int argc, char** argv);

namespace Strontium
{
  class Model;
  class Material;

  // Singleton application class.
  class Application
  {
  public:
    Application(const std::string &name = "Editor Viewport");
    virtual ~Application();

    // Push layers and overlays.
    void pushLayer(Layer* layer);
    void pushOverlay(Layer* overlay);

    // Close the application.
    void close();

    // Getters.
    static Application* getInstance() { return Application::appInstance; }
    Shared<Window> getWindow() { return this->appWindow; }
    bool isRunning() { return this->running; }
    AssetManager& getAssetCache() { return this->assetCache; }

  protected:
    static Application* appInstance;
    AssetManager assetCache;
    bool running, isMinimized;
    std::string name;

    LayerCollection layerStack;
    ImGuiLayer* imLayer;
    Shared<Window> appWindow;

    float lastTime;
  private:
    friend int ::main(int argc, char** argv);

    void run();
    void dispatchEvents();
    void onEvent(Event &event);
    void onWindowResize();
  };

  // Need to define this in the client app.
  Application* makeApplication();
}
