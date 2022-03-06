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

  protected:
    // The application instance.
    static Application* appInstance;

    // Determines if the application should continue to run or not.
    bool running, isMinimized;

    // Application name.
    std::string name;

    // The layer stack for the application.
    LayerCollection layerStack;

    // The main application window.
    Shared<Window> appWindow;

    // The ImGui layer. Implements the ImGui boilerplate behavior.
    ImGuiLayer* imLayer;

    // The last frame time.
    float lastTime;

    // Asset managers for the different assets loaded in.
    Unique<AssetManager<Model>> modelAssets;
    Unique<AssetManager<Material>> materialAssets;
    Unique<AssetManager<Texture2D>> texture2DAssets;
  private:
    // The main function.
    friend int ::main(int argc, char** argv);

    // Functions for application behavior.
    void run();
    void dispatchEvents();
    void onEvent(Event &event);
    void onWindowResize();
  };

  // Need to define this in the client app.
  Application* makeApplication();
}
