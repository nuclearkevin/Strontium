#include "EditorLayer.h"

// Project includes.
#include "Core/Application.h"
#include "Core/Logs.h"
#include "Core/KeyCodes.h"
#include "GuiElements/Styles.h"
#include "GuiElements/Panels.h"
#include "Serialization/YamlSerialization.h"
#include "Scenes/Components.h"

#include "PhysicsEngine/PhysicsEngine.h"

namespace Strontium
{
  EditorLayer::EditorLayer()
    : Layer("Editor Layer")
    , editorCam(1920 / 2, 1080 / 2, glm::vec3{ 0.0f, 1.0f, 4.0f }, 
                EditorCameraType::Stationary)
    , loadTarget(FileLoadTargets::TargetNone)
    , saveTarget(FileSaveTargets::TargetNone)
    , dndScenePath("")
    , showPerf(true)
    , editorSize(ImVec2(0, 0))
    , sceneState(SceneState::Edit)
  { }

  EditorLayer::~EditorLayer()
  { }

  void
  EditorLayer::onAttach()
  {
    Styles::setDefaultTheme();

    // Fetch the width and height of the window and create a floating point
    // framebuffer.
    glm::ivec2 wDims = Application::getInstance()->getWindow()->getSize();
    this->drawBuffer.resize(static_cast<uint>(wDims.x), static_cast<uint>(wDims.y));
    this->drawBuffer.setClearColour(glm::vec4(0.0f));

    auto cSpec = Texture2D::getFloatColourParams();
    auto colourAttachment = FBOAttachment(FBOTargetParam::Colour0, FBOTextureParam::Texture2D,
                                          cSpec.internal, cSpec.format, cSpec.dataType);
    this->drawBuffer.attach(cSpec, colourAttachment);
    
    cSpec.internal = TextureInternalFormats::R32f;
    cSpec.format = TextureFormats::Red;
    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    colourAttachment = FBOAttachment(FBOTargetParam::Colour1, FBOTextureParam::Texture2D,
                                     cSpec.internal, cSpec.format, cSpec.dataType);
    this->drawBuffer.attach(cSpec, colourAttachment);

    auto dSpec = Texture2D::getDefaultDepthParams();
    auto depthAttachment = FBOAttachment(FBOTargetParam::Depth, FBOTextureParam::Texture2D,
                                           dSpec.internal, dSpec.format, dSpec.dataType);
  	this->drawBuffer.attach(dSpec, depthAttachment);
    this->drawBuffer.setDrawBuffers();

    // Setup stuff for the scene.
    this->currentScene = createShared<Scene>();
    auto entity = this->currentScene->createEntity("Skylight");
    entity.addComponent<TransformComponent>();
    entity.addComponent<DirectionalLightComponent>().intensity = 10.0f;
    entity.addComponent<SkyAtmosphereComponent>();
    entity.addComponent<DynamicSkyboxComponent>();
    entity.addComponent<DynamicSkylightComponent>();

    this->backupScene = createShared<Scene>();

    // Init the editor camera and windows.
    this->editorCam.init(90.0f, 1.0f, 0.1f, 200.0f);
    this->windowManager.insertWindow<SceneGraphWindow>(this);
    this->windowManager.insertWindow<CameraWindow>(this, &this->editorCam);
    this->windowManager.insertWindow<ShaderWindow>(this);
    this->windowManager.insertWindow<FileBrowserWindow>(this);
    this->windowManager.insertWindow<ModelWindow>(this, false);
    this->windowManager.insertWindow<AssetBrowserWindow>(this);
    this->windowManager.insertWindow<RendererWindow>(this);
    this->windowManager.insertWindow<ViewportWindow>(this);
  }

  void
  EditorLayer::onDetach()
  {

  }

  // On event for the layer.
  void
  EditorLayer::onEvent(Event &event)
  {
    // Push the events through to all the gui elements.
    this->windowManager.onEvent(event);

    // Push the event through to the editor camera.
    this->editorCam.onEvent(event);

    // Handle events for the layer.
    switch(event.getType())
    {
      case EventType::KeyPressedEvent:
      {
        auto keyEvent = *(static_cast<KeyPressedEvent*>(&event));
        this->onKeyPressEvent(keyEvent);
        break;
      }

      case EventType::MouseClickEvent:
      {
        auto mouseEvent = *(static_cast<MouseClickEvent*>(&event));
        this->onMouseEvent(mouseEvent);
        break;
      }

      case EventType::EntityDeleteEvent:
      {
        auto entDeleteEvent = *(static_cast<EntityDeleteEvent*>(&event));
        auto entityID = entDeleteEvent.getStoredEntity();
        auto entityParentScene = entDeleteEvent.getStoredScene();

        this->currentScene->recurseDeleteEntity(Entity((entt::entity) entityID, entityParentScene));

        break;
      }

      case EventType::LoadFileEvent:
      {
        auto loadEvent = *(static_cast<LoadFileEvent*>(&event));

        if (this->loadTarget == FileLoadTargets::TargetScene)
        {
          Shared<Scene> tempScene = createShared<Scene>();
          bool success = YAMLSerialization::deserializeScene(tempScene, loadEvent.getAbsPath());

          if (success)
          {
            Logs::log("Scene loaded from: " + loadEvent.getAbsPath());
            this->currentScene = tempScene;
            this->currentScene->getSaveFilepath() = loadEvent.getAbsPath();
            this->windowManager.getWindow<SceneGraphWindow>()->setSelectedEntity(Entity());
            this->windowManager.getWindow<ModelWindow>()->setSelectedEntity(Entity());
          }
        }

        this->loadTarget = FileLoadTargets::TargetNone;
        break;
      }

      case EventType::SaveFileEvent:
      {
        auto saveEvent = *(static_cast<SaveFileEvent*>(&event));

        if (this->saveTarget == FileSaveTargets::TargetScene)
        {
          std::string name = saveEvent.getFileName().substr(0, saveEvent.getFileName().find_last_of('.'));
          YAMLSerialization::serializeScene(this->currentScene, saveEvent.getAbsPath(), name);
          this->currentScene->getSaveFilepath() = saveEvent.getAbsPath();

          Logs::log("Scene saved at: " + saveEvent.getAbsPath());
        }

        this->saveTarget = FileSaveTargets::TargetNone;
        break;
      }

      default: break;
    }
  }

  // On update for the layer.
  void
  EditorLayer::onUpdate(float dt)
  {
    // Update each of the windows.
    this->windowManager.onUpdate(dt, this->currentScene);

    // Update the size of the framebuffer to fit the editor window.
    glm::vec2 size = this->drawBuffer.getSize();
    if (this->editorSize.x != size.x || this->editorSize.y != size.y)
    {
      if (this->editorSize.x >= 1.0f && this->editorSize.y >= 1.0f)
        this->editorCam.updateProj(this->editorCam.getHorFOV(),
                                   this->editorSize.x / this->editorSize.y,
                                   this->editorCam.getNear(),
                                   this->editorCam.getFar());
      this->drawBuffer.resize(this->editorSize.x, this->editorSize.y);
    }

    // Update the scene.
    switch (this->sceneState)
    {
      case SceneState::Edit:
      {
        // Update the scene.
        this->currentScene->onUpdateEditor(dt);

        this->drawBuffer.clear();
        // Draw the scene.
        Renderer3D::begin(this->editorSize.x, this->editorSize.y, static_cast<Camera>(this->editorCam), dt);
        this->currentScene->onRenderEditor(this->editorSize.x / this->editorSize.y, this->getSelectedEntity());
        Renderer3D::end(this->drawBuffer);

        // Draw debug information.
        DebugRenderer::begin(this->editorSize.x, this->editorSize.y, static_cast<Camera>(this->editorCam));
        this->currentScene->onRenderDebug(this->editorSize.x / this->editorSize.y);
        DebugRenderer::end(this->drawBuffer);

        // Update the editor camera.
        this->editorCam.onUpdate(dt, glm::vec2(this->editorSize.x, this->editorSize.y));

        break;
      }

      case SceneState::Play:
      {
        // Update the scene.
        this->currentScene->onUpdateRuntime(dt);

        // Update the physics system.
        this->currentScene->simulatePhysics(dt);

        // Fetch the primary camera entity.
        auto primaryCameraEntity = this->currentScene->getPrimaryCameraEntity();
        Camera primaryCamera;
        if (primaryCameraEntity)
        {
          auto transform = this->currentScene->computeGlobalTransform(primaryCameraEntity);
          primaryCamera = primaryCameraEntity.getComponent<CameraComponent>().entCamera;

          primaryCamera.position = glm::vec3(transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
          primaryCamera.front = glm::normalize(glm::vec3(transform  * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

          primaryCamera.view = glm::lookAt(primaryCamera.position, primaryCamera.position + primaryCamera.front, 
                                           glm::vec3(0.0f, 1.0f, 0.0f));

          primaryCamera.projection = glm::perspective(primaryCamera.fov,
                                                      this->editorSize.x / this->editorSize.y, 
                                                      primaryCamera.near,
                                                      primaryCamera.far);
          primaryCamera.invViewProj = glm::inverse(primaryCamera.projection * primaryCamera.view);
        }
        else
        {
          // Fall back to the editor camera if now primary camera is available.
          primaryCamera = static_cast<Camera>(this->editorCam);
          this->editorCam.onUpdate(dt, glm::vec2(this->editorSize.x, this->editorSize.y));
        }

        this->drawBuffer.clear();
        // Draw the scene.
        Renderer3D::begin(this->editorSize.x, this->editorSize.y, primaryCamera, dt);
        this->currentScene->onRenderRuntime(this->editorSize.x / this->editorSize.y);
        Renderer3D::end(this->drawBuffer);

        // Draw debug information.
        DebugRenderer::begin(this->editorSize.x, this->editorSize.y, primaryCamera);
        this->currentScene->onRenderDebug(this->editorSize.x / this->editorSize.y);
        DebugRenderer::end(this->drawBuffer);

        break;
      }

      case SceneState::Simulate:
      {
        // Update the scene.
        this->currentScene->onUpdateEditor(dt);

        // Update the physics system.
        this->currentScene->simulatePhysics(dt);

        this->drawBuffer.clear();
        // Draw the scene.
        Renderer3D::begin(this->editorSize.x, this->editorSize.y, static_cast<Camera>(this->editorCam), dt);
        this->currentScene->onRenderEditor(this->editorSize.x / this->editorSize.y, this->getSelectedEntity());
        Renderer3D::end(this->drawBuffer);

        // Draw debug information.
        DebugRenderer::begin(this->editorSize.x, this->editorSize.y, static_cast<Camera>(this->editorCam));
        this->currentScene->onRenderDebug(this->editorSize.x / this->editorSize.y);
        DebugRenderer::end(this->drawBuffer);

        // Update the editor camera.
        this->editorCam.onUpdate(dt, glm::vec2(this->editorSize.x, this->editorSize.y));
        break;
      }
    }
  }

  void
  EditorLayer::onImGuiRender()
  {
    // Get the window size.
    ImVec2 wSize = ImGui::GetIO().DisplaySize;

    static bool dockspaceOpen = true;

    // Seting up the dockspace.
    // Set the size of the full screen ImGui window to the size of the viewport.
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Window flags to prevent the docking context from taking up visible space.
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // Dockspace flags.
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

    // Remove window sizes and create the window.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, windowFlags);
	ImGui::PopStyleVar(3);

    // Prepare the dockspace context.
    ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 370.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
	  ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
	  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
	}

	style.WindowMinSize.x = minWinSizeX;

    // On ImGui render methods for all the GUI elements.
    this->windowManager.onImGuiRender(this->currentScene);

  	if (ImGui::BeginMainMenuBar())
  	{
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem(ICON_FA_FILE_O" New", "Ctrl+N"))
       	{
          this->currentScene = createShared<Scene>();
       	}
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN_O" Open...", "Ctrl+O"))
       	{
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                       ".srn"));
          this->loadTarget = FileLoadTargets::TargetScene;
       	}
        if (ImGui::MenuItem(ICON_FA_FLOPPY_O" Save", "Ctrl+S"))
        {
          if (this->currentScene->getSaveFilepath() != "")
          {
            std::string path =  this->currentScene->getSaveFilepath();
            std::string name = path.substr(path.find_last_of('/') + 1, path.find_last_of('.'));

            YAMLSerialization::serializeScene(this->currentScene, path, name);
            Logs::log("Scene saved at: " + path);
          }
          else
          {
            EventDispatcher* dispatcher = EventDispatcher::getInstance();
            dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                         ".srn"));
            this->saveTarget = FileSaveTargets::TargetScene;
          }
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_O" Save As", "Ctrl+Shift+S"))
       	{
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                       ".srn"));
          this->saveTarget = FileSaveTargets::TargetScene;
       	}
        if (ImGui::MenuItem(ICON_FA_POWER_OFF" Exit"))
        {
          EventDispatcher* appEvents = EventDispatcher::getInstance();
          appEvents->queueEvent(new WindowCloseEvent());
        }

       	ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Edit")) 
        ImGui::EndMenu();

      if (ImGui::BeginMenu("Add")) 
        ImGui::EndMenu();

      if (ImGui::BeginMenu("Scripts")) 
        ImGui::EndMenu();

      if (ImGui::BeginMenu("Settings"))
      {
        if (ImGui::BeginMenu("Menus"))
        {
          if (ImGui::BeginMenu("Scene Menu Settings"))
          {
            if (ImGui::MenuItem("Show Scene Graph"))
              this->windowManager.getWindow<SceneGraphWindow>()->isOpen = true;

            if (ImGui::MenuItem("Show Model Information"))
              this->windowManager.getWindow<ModelWindow>()->isOpen = true;

            ImGui::EndMenu();
          }

          if (ImGui::BeginMenu("Editor Menu Settings"))
          {
            if (ImGui::MenuItem("Show Content Browser"))
              this->windowManager.getWindow<AssetBrowserWindow>()->isOpen = true;

            if (ImGui::MenuItem("Show Performance Stats Menu"))
              this->showPerf = true;

            if (ImGui::MenuItem("Show Camera Menu"))
              this->windowManager.getWindow<CameraWindow>()->isOpen = true;

            if (ImGui::MenuItem("Show Shader Menu"))
              this->windowManager.getWindow<ShaderWindow>()->isOpen = true;

            ImGui::EndMenu();
          }

          if (ImGui::MenuItem("Show Renderer Settings"))
            this->windowManager.getWindow<RendererWindow>()->isOpen = true;

          ImGui::EndMenu();
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help"))
        ImGui::EndMenu();

      ImGui::EndMainMenuBar();
  	}

    // The log menu.
    ImGui::Begin("Application Logs", nullptr);
    {
      ImGui::BeginChild("LogText");
      {
        auto size = ImGui::GetWindowSize();
        ImGui::PushTextWrapPos(size.x);
        auto messageCount = Logs::getMessageCount();
        auto messagePointer = Logs::getMessagePointer();
        if (messageCount == 50)
        {
          for (std::size_t i = 0; i < 50; i++)
          {
            auto index = (messagePointer + 1 + i) % 50;
            auto message = Logs::getMessage(index);
            ImGui::Text(message.c_str());
          }
        }
        else
        {
          for (std::size_t i = 0; i < messagePointer; i++)
          {
            auto message = Logs::getMessage(i);
            ImGui::Text(message.c_str());
          }
        }
        ImGui::PopTextWrapPos();
      }
      ImGui::EndChild();
    }
    ImGui::End();

    // The performance window.
    if (this->showPerf)
    {
      ImGui::Begin("Performance Window", &this->showPerf);
      auto size = ImGui::GetWindowSize();
      ImGui::PushTextWrapPos(size.x);
      ImGui::Text(Application::getInstance()->getWindow()->getContextInfo().c_str());
      ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::PopTextWrapPos();
      ImGui::End();
    }

    // Toolbar window.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    auto& colors = ImGui::GetStyle().Colors;
    const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
    const auto& buttonActive = colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

    ImGui::Begin("##buttonBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    float size = ImGui::GetWindowHeight() - 4.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - size);
    if (this->sceneState == SceneState::Simulate)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button(ICON_FA_PLAY, ImVec2(size, size));
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
    {
      if (ImGui::Button(this->sceneState == SceneState::Edit ? ICON_FA_PLAY : ICON_FA_STOP, ImVec2(size, size)))
      {
        if (this->sceneState == SceneState::Edit)
          this->onScenePlay();
        else if (this->sceneState == SceneState::Play)
          this->onSceneStop();
      }
    }

    //ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) + (size * 1.5f));
    ImGui::SameLine();
    if (this->sceneState == SceneState::Play)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button(ICON_FA_FORWARD, ImVec2(size, size));
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
    {
      if (ImGui::Button(this->sceneState == SceneState::Edit ? ICON_FA_FORWARD : ICON_FA_STOP, ImVec2(size, size)))
      {
        if (this->sceneState == SceneState::Edit)
          this->onSceneSimulate();
        else if (this->sceneState == SceneState::Simulate)
          this->onSceneStop();
      }
    }
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    ImGui::End();

    // Show a warning when a new scene is to be loaded.
    if (this->dndScenePath != "" && this->currentScene->getRegistry().size() > 0)
    {
      auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

      ImGui::Begin("Warning", nullptr, flags);

      ImGui::Text("Loading a new scene will overwrite the current scene, do you wish to continue?");
      ImGui::Text(" ");

      auto cursor = ImGui::GetCursorPos();
      ImGui::SetCursorPos(ImVec2(cursor.x + 90.0f, cursor.y));
      if (ImGui::Button("Save and Continue"))
      {
        if (this->currentScene->getSaveFilepath() != "")
        {
          std::string path =  this->currentScene->getSaveFilepath();
          std::string name = path.substr(path.find_last_of('/') + 1, path.find_last_of('.'));

          YAMLSerialization::serializeScene(this->currentScene, path, name);
          Logs::log("Scene saved at: " + path);

          this->windowManager.getWindow<SceneGraphWindow>()->setSelectedEntity(Entity());
          this->windowManager.getWindow<ModelWindow>()->setSelectedEntity(Entity());

          Shared<Scene> tempScene = createShared<Scene>();
          if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
          {
            this->currentScene = tempScene;
            this->currentScene->getSaveFilepath() = this->dndScenePath;
            Logs::log("Scene loaded from: " + path);
          }
          this->dndScenePath = "";
        }
        else
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                       ".srn"));
          this->saveTarget = FileSaveTargets::TargetScene;
        }
      }

      ImGui::SameLine();
      if (ImGui::Button("Continue"))
      {
        this->windowManager.getWindow<SceneGraphWindow>()->setSelectedEntity(Entity());
        this->windowManager.getWindow<ModelWindow>()->setSelectedEntity(Entity());

        Shared<Scene> tempScene = createShared<Scene>();
        if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
        {
          this->currentScene = tempScene;
          this->currentScene->getSaveFilepath() = this->dndScenePath;
          Logs::log("Scene loaded from: " + this->dndScenePath);
        }
        this->dndScenePath = "";
      }

      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
        this->dndScenePath = "";

      ImGui::End();
    }
    else if (this->dndScenePath != "")
    {
      this->windowManager.getWindow<SceneGraphWindow>()->setSelectedEntity(Entity());
      this->windowManager.getWindow<ModelWindow>()->setSelectedEntity(Entity());

      Shared<Scene> tempScene = createShared<Scene>();
      if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
      {
        this->currentScene = tempScene;
        this->currentScene->getSaveFilepath() = this->dndScenePath;
        Logs::log("Scene loaded from: " + this->dndScenePath);
      }
      this->dndScenePath = "";
    }

    // MUST KEEP THIS. Docking window end.
    ImGui::End();
  }

  void
  EditorLayer::onKeyPressEvent(KeyPressedEvent &keyEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int keyCode = keyEvent.getKeyCode();

    bool lControlHeld = appWindow->isKeyPressed(SR_KEY_LEFT_CONTROL);
    bool lShiftHeld = appWindow->isKeyPressed(SR_KEY_LEFT_SHIFT);

    switch (keyCode)
    {
      case SR_KEY_N:
      {
        if (lControlHeld && keyEvent.getRepeatCount() == 0)
        {
          this->currentScene = createShared<Scene>();
          this->windowManager.getWindow<SceneGraphWindow>()->setSelectedEntity(Entity());
          this->windowManager.getWindow<ModelWindow>()->setSelectedEntity(Entity());

          Logs::log("New scene created.");
        }
        break;
      }
      case SR_KEY_O:
      {
        if (lControlHeld && keyEvent.getRepeatCount() == 0)
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                       ".srn"));
          this->loadTarget = FileLoadTargets::TargetScene;
        }
        break;
      }
      case SR_KEY_S:
      {
        if (lControlHeld && lShiftHeld && keyEvent.getRepeatCount() == 0)
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                       ".srn"));
          this->saveTarget = FileSaveTargets::TargetScene;
        }
        else if (lControlHeld && keyEvent.getRepeatCount() == 0)
        {
          if (this->currentScene->getSaveFilepath() != "")
          {
            std::string path =  this->currentScene->getSaveFilepath();
            std::string name = path.substr(path.find_last_of('/') + 1, path.find_last_of('.'));

            YAMLSerialization::serializeScene(this->currentScene, path, name);

            Logs::log("Scene saved at: " + path);
          }
          else
          {
            EventDispatcher* dispatcher = EventDispatcher::getInstance();
            dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                         ".srn"));
            this->saveTarget = FileSaveTargets::TargetScene;
          }
        }
        break;
      }
    }
  }

  void EditorLayer::onMouseEvent(MouseClickEvent &mouseEvent)
  {
    
  }

  void
  EditorLayer::onScenePlay()
  {
    this->sceneState = SceneState::Play;

    // Copy the current scene to the editor scene.
    this->backupScene->clearForRuntime();
    this->backupScene->copyForRuntime(*this->currentScene);
    this->backupScene->getSaveFilepath() = this->currentScene->getSaveFilepath();

    this->currentScene->initPhysics();
  }

  void
  EditorLayer::onSceneSimulate()
  {
    this->sceneState = SceneState::Simulate;

    // Copy the current scene to the editor scene.
    this->backupScene->clearForRuntime();
    this->backupScene->copyForRuntime(*this->currentScene);
    this->backupScene->getSaveFilepath() = this->currentScene->getSaveFilepath();

    this->currentScene->initPhysics();
  }

  void
  EditorLayer::onSceneStop()
  {
    this->sceneState = SceneState::Edit;

    this->currentScene->shutdownPhysics();

    // Copy the editor scene to the current scene 
    this->currentScene->clearForRuntime();
    this->currentScene->copyForRuntime(*this->backupScene);
    this->currentScene->getSaveFilepath() = this->backupScene->getSaveFilepath();
  }

  Entity
  EditorLayer::getSelectedEntity()
  {
    return this->windowManager.getWindow<SceneGraphWindow>()->getSelectedEntity();
  }
}
