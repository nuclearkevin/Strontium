#include "Core/Application.h"

// Project includes.
#include "Core/Events.h"
#include "Core/Logs.h"

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
      assert(Application::appInstance != nullptr);
    }

    Application::appInstance = this;

    // Initialize the application logs.
    SciRenderer::Logger* logs = SciRenderer::Logger::getInstance();
    logs->init();

    // Initialize the application main window.
    this->appWindow = Window::getNewInstance(this->name);

    // Initialize the thread pool.
    workerGroup = Unique<ThreadPool>(ThreadPool::getInstance(4));

    // Initialize the asset managers.
    this->shaderCache.reset(AssetManager<Shader>::getManager());
    this->modelAssets.reset(AssetManager<Model>::getManager());
    this->texture2DAssets.reset(AssetManager<Texture2D>::getManager());

    // Load the shaders into a cache.
    this->shaderCache->attachAsset("pbr_shader",
      new Shader("./assets/shaders/mesh.vs",
                 "./assets/shaders/forward/pbr/pbrTex.fs"));

    this->shaderCache->attachAsset("shadow_shader",
      new Shader("./assets/shaders/shadows/directionalShadow.vs",
                 "./assets/shaders/shadows/directionalShadow.fs"));

    this->shaderCache->attachAsset("geometry_pass_shader",
      new Shader("./assets/shaders/deferred/geometryPass.vs",
                 "./assets/shaders/deferred/geometryPass.fs"));

    this->shaderCache->attachAsset("deferred_ambient",
      new Shader("./assets/shaders/deferred/lightingPass.vs",
                 "./assets/shaders/deferred/ambientLightingPass.fs"));

    this->shaderCache->attachAsset("deferred_directional",
      new Shader("./assets/shaders/deferred/lightingPass.vs",
                 "./assets/shaders/deferred/directionalLightPass.fs"));

    this->shaderCache->attachAsset("post_hdr",
      new Shader("./assets/shaders/post/postProcessingPass.vs",
                 "./assets/shaders/post/hdrPostPass.fs"));

    this->shaderCache->attachAsset("post_entity_outline",
      new Shader("./assets/shaders/post/postProcessingPass.vs",
                 "./assets/shaders/post/outlinePostPass.fs"));

    this->shaderCache->attachAsset("post_hor_gaussian_blur",
      new Shader("./assets/shaders/post/postProcessingPass.vs",
                 "./assets/shaders/post/horShadowBlur.fs"));

    this->shaderCache->attachAsset("post_ver_gaussian_blur",
      new Shader("./assets/shaders/post/postProcessingPass.vs",
                 "./assets/shaders/post/verShadowBlur.fs"));

    this->shaderCache->attachAsset("fsq_shader",
      new Shader("./assets/shaders/viewport.vs",
                 "./assets/shaders/viewport.fs"));

    // Load the default assets.
    this->texture2DAssets->setDefaultAsset(Texture2D::createMonoColour(
      glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), Texture2DParams(), false));
    Texture2D::createMonoColour(glm::vec4(1.0f));
    Texture2D::createMonoColour(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));

    this->imLayer = new ImGuiLayer();
    this->pushOverlay(this->imLayer);

    // Initialize the 3D renderer.
    Renderer3D::init(1600.0f, 900.0f);
  }

  Application::~Application()
  {
    // Detach each layer and delete it.
    for (auto layer : this->layerStack)
		{
  		layer->onDetach();
  		delete layer;
		}

    // Shutdown the renderer.
    Renderer3D::shutdown();

    // Delete the application event dispatcher and logs.
    delete EventDispatcher::getInstance();
    delete Logger::getInstance();
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

        // Clear the back buffer.
        RendererCommands::clear(true, false, false);
      }

      // Must be called at the end of every frame to create textures with loaded
      // images.
      Texture2D::bulkGenerateTextures();
      Model::bulkGenerateMaterials();
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
      this->isMinimized = true;
    else
      this->isMinimized = false;
  }
}
