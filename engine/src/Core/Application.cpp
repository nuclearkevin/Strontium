#include "Core/Application.h"

// Project includes.
#include "Core/JobSystem.h"
#include "Core/Events.h"
#include "Core/Logs.h"

#include "Assets/ModelAsset.h"
#include "Assets/Image2DAsset.h"
#include "Assets/MaterialAsset.h"
#include "Utils/AsyncAssetLoading.h"

#include "Graphics/RendererCommands.h"

#include "PhysicsEngine/PhysicsEngine.h"

namespace Strontium
{
  Application* Application::appInstance = nullptr;

  // Singleton application class for everything that happens in Strontium.
  Application::Application(const std::string &name)
    : name(name)
    , running(true)
    , isMinimized(false)
    , lastTime(0.0f)
    , assetCache("./assets/registry.yaml")
  {
    assert(("Error: Already have an instance of the application.", Application::appInstance == nullptr));

    Application::appInstance = this;

    // Initialize the application logs.
    Logs::init("./logs.txt");

    // Initialize the application main window.
    this->appWindow = Window::getNewInstance(this->name);

    // Initialize the thread pool.
    JobSystem::init(3);

    // Init the shader cache.
    ShaderCache::init("./assets/shaders/shaderManifest.yaml");

    // Initialize the renderers.
    Renderer3D::init(1600u, 900u);

    // Init the physics system.
    PhysicsEngine::init();

    // Load the default assets.
    // Default texture (an ugly purple) and the default material properties
    // texture (white) and default normal map.
    auto image2DDefault = this->assetCache.emplaceDefaultAsset<Image2DAsset>();
    Texture2D::createMonoColour(*(image2DDefault->getTexture()), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    Texture2D::createMonoColour(glm::vec4(1.0f));
    Texture2D::createMonoColour(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));

    // Default material.
    this->assetCache.emplaceDefaultAsset<MaterialAsset>();

    this->imLayer = new ImGuiLayer();
    this->pushOverlay(this->imLayer);
  }

  Application::~Application()
  {
    // Detach each layer and delete it.
    for (auto layer : this->layerStack)
	{
  	  layer->onDetach();
  	  delete layer;
	}

    // Shutdown the renderers.
    Renderer3D::shutdown();

    // Shutdown the physics system.
    PhysicsEngine::shutdown();

    // Terminate the spawned threads.
    JobSystem::shutdown();

    // Shutdown the logs.
    Logs::shutdown();

    // Delete the application event dispatcher and logs.
    delete EventDispatcher::getInstance();
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
      float currentTime = this->appWindow->getTime();
      float deltaTime = currentTime - this->lastTime;
      this->lastTime = currentTime;

      // Handle application events
      this->dispatchEvents();

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

          // Update the window.
          this->appWindow->onUpdate();

          // Clear the back buffer.
          RendererCommands::clear(true, false, false);
      }

      if (this->isMinimized)
        this->appWindow->onUpdate();

      // Must be called at the end of every frame to create textures with loaded
      // images.
      AsyncLoading::bulkGenerateTextures();
      AsyncLoading::bulkGenerateMaterials();
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

    if (event.getType() == EventType::WindowCloseEvent)
      this->close();
  }

  void
  Application::onWindowResize()
  {
    glm::ivec2 windowSize = getWindow()->getSize();

    if (windowSize.x == 0 || windowSize.y == 0)
    {
      this->isMinimized = true;
    }
    else
    {
      this->isMinimized = false;
    }
  }
}
