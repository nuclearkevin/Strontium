#include "Core/Application.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Core/Events.h"

namespace SciRenderer
{
  Application* Application::appInstance = nullptr;

  // Singleton application class for everything that happens in SciRender.
  Application::Application(const std::string &name)
    : name(name)
    , running(true)
    , isMinimized(false)
    , lastTime(0.0f)
  {
    if (Application::appInstance != nullptr)
    {
      std::cout << "Already have an instance of the application. Aborting"
                << std::endl;
      exit(EXIT_FAILURE);
    }

    Application::appInstance = this;

    this->appWindow = Window::getNewInstance(this->name);

    Renderer3D* renderer = Renderer3D::getInstance();
    renderer->init("./res/shaders/viewport.vs", "./res/shaders/viewport.fs");

    this->imLayer = new ImGuiLayer();
    this->pushOverlay(this->imLayer);
  }

  Application::~Application()
  {
    // Don't need to loop over the layers, the layer collection handles that in
    // its destructor.

    // Shutdown and delete the application window.
    delete this->appWindow;
  }

  void
  Application::pushLayer(Layer* layer)
  {
    this->layerStack.pushLayer(layer);
    layer->onAttach();
  }

  void
  Application::pushOverlay(Layer* overlay)
  {
    this->layerStack.pushOverlay(overlay);
    overlay->onAttach();
  }

  void
  Application::close()
  {
    this->running = false;
  }

  // The main application run function with the run loop.
  void Application::run()
  {
    while (this->running)
    {
      // Fetch delta time.
      float currentTime = glfwGetTime();
      float deltaTime = currentTime - this->lastTime;
      this->lastTime = currentTime;

      // Make sure the window isn't minimized to avoid losing performance.
      if (!this->isMinimized)
      {
        // Loop over each layer and call its update function.
        for (auto layer : this->layerStack)
          layer->onUpdate(deltaTime);

        // Setup ImGui for drawing, than loop over each layer and draw its GUI
        // elements.
        this->imLayer->beginImGui();
        for (auto layer : this->layerStack)
          layer->onImGuiRender();

        this->imLayer->endImGui();

        // Handle application events
        this->dispatchEvents();

        // Update the window.
        this->appWindow->onUpdate();
      }
    }
  }

  void
  Application::dispatchEvents()
  {
    // Fetch the dispatcher.
    EventDispatcher* appEvents = EventDispatcher::getInstance();

    while (!appEvents->isEmpty())
    {
      // Fetch the first event.
      Event* event = appEvents->dequeueEvent();

      // Call the application on event function first.
      this->onEvent(*event);

      // Call the on event functions for each layer.
      for (auto layer : this->layerStack)
        layer->onEvent(*event);

      // Delete the event when we're done with it.
      Event::deleteEvent(event);
    }
  }

  void
  Application::onEvent(Event &event)
  {
    if (event.getType() == EventType::WindowResizeEvent)
      this->onWindowResize();
  }

  void
  Application::onWindowResize()
  {
    glm::ivec2 windowSize = getWindow()->getSize();

    if (windowSize.x == 0 || windowSize.y == 0)
      this->isMinimized = true;
    else
      this->isMinimized = false;
  }
}
