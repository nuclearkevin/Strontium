#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Layers/Layers.h"
#include "Core/Window.h"
#include "Layers/ImGuiLayer.h"

int main(int argc, char** argv);

namespace SciRenderer
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
    static inline Application* getInstance() { return Application::appInstance; }
    inline Shared<Window> getWindow() { return this->appWindow; }

  private:
    // The application instance.
    static Application* appInstance;

    // The main function.
    friend int ::main(int argc, char** argv);

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

  private:
    // Functions for application behavior.
    void run();
    void dispatchEvents();
    void onEvent(Event &event);
    void onWindowResize();
  };

  // Need to define this in the client app.
  Application* makeApplication();
}
