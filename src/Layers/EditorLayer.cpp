#include "Layers/EditorLayer.h"

// Project includes.
#include "Core/Application.h"
#include "Core/Logs.h"
#include "GuiElements/Styles.h"
#include "GuiElements/Panels.h"
#include "Serialization/YamlSerialization.h"
#include "Scenes/Components.h"

// Some math for decomposing matrix transformations.
#include "glm/gtx/matrix_decompose.hpp"

// ImGizmo goodies.
#include "imguizmo/ImGuizmo.h"

namespace Strontium
{
  EditorLayer::EditorLayer()
    : Layer("Editor Layer")
    , loadTarget(FileLoadTargets::TargetNone)
    , saveTarget(FileSaveTargets::TargetNone)
    , dndScenePath("")
    , showPerf(true)
    , editorSize(ImVec2(0, 0))
  { }

  EditorLayer::~EditorLayer()
  {
    for (auto& window : this->windows)
      delete window;
  }

  void
  EditorLayer::onAttach()
  {
    Styles::setDefaultTheme();

    // Fetch the width and height of the window and create a floating point
    // framebuffer.
    glm::ivec2 wDims = Application::getInstance()->getWindow()->getSize();
    this->drawBuffer = createShared<FrameBuffer>((GLuint) wDims.x, (GLuint) wDims.y);

    // Fetch a default floating point FBO spec and attach it. Also attach a single
    // float spec for entity IDs.
    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    this->drawBuffer->attachTexture2D(cSpec);
    cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1); // The ID texture.
    cSpec.internal = TextureInternalFormats::R32f;
    cSpec.format = TextureFormats::Red;
    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    this->drawBuffer->attachTexture2D(cSpec);
    this->drawBuffer->setDrawBuffers();
  	this->drawBuffer->attachTexture2D(FBOCommands::getDefaultDepthSpec());

    // Setup stuff for the scene.
    this->currentScene = createShared<Scene>();

    // Finally, the editor camera.
    this->editorCam = createShared<Camera>(1920 / 2, 1080 / 2, glm::vec3 { 0.0f, 1.0f, 4.0f },
                                           EditorCameraType::Stationary);
    this->editorCam->init(90.0f, 1.0f, 0.1f, 200.0f);

    // All the windows!
    this->windows.push_back(new SceneGraphWindow(this));
    this->windows.push_back(new CameraWindow(this, this->editorCam));
    this->windows.push_back(new ShaderWindow(this));
    this->windows.push_back(new FileBrowserWindow(this));
    this->windows.push_back(new ModelWindow(this));
    this->windows.push_back(new AssetBrowserWindow(this));
    this->windows.push_back(new RendererWindow(this)); // 6
    this->windows.push_back(new ViewportWindow(this));
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
    for (auto& window : this->windows)
      window->onEvent(event);

    // Push the event through to the editor camera.
    this->editorCam->onEvent(event);

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
            this->currentScene = tempScene;
            this->currentScene->getSaveFilepath() = loadEvent.getAbsPath();
            static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
            static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());
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

          if (this->dndScenePath != "")
          {
            static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
            static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

            Shared<Scene> tempScene = createShared<Scene>();
            if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
            {
              this->currentScene = tempScene;
              this->currentScene->getSaveFilepath() = this->dndScenePath;
            }
            this->dndScenePath = "";
          }
        }

        this->saveTarget = FileSaveTargets::TargetNone;
        break;
      }

      default:
      {
        break;
      }
    }
  }

  // On update for the layer.
  void
  EditorLayer::onUpdate(float dt)
  {
    // Update each of the windows.
    for (auto& window : this->windows)
      window->onUpdate(dt, this->currentScene);

    // Update the size of the framebuffer to fit the editor window.
    glm::vec2 size = this->drawBuffer->getSize();
    if (this->editorSize.x != size.x || this->editorSize.y != size.y)
    {
      if (this->editorSize.x >= 1.0f && this->editorSize.y >= 1.0f)
        this->editorCam->updateProj(this->editorCam->getHorFOV(),
                                    editorSize.x / editorSize.y,
                                    this->editorCam->getNear(),
                                    this->editorCam->getFar());
      this->drawBuffer->resize(this->editorSize.x, this->editorSize.y);
    }

    // Update the scene.
    this->currentScene->onUpdate(dt);

    // Draw the scene.
    Renderer3D::begin(this->editorSize.x, this->editorSize.y, this->editorCam, false);
    this->currentScene->render(this->getSelectedEntity());
    Renderer3D::end(this->drawBuffer);

    // Update the editor camera.
    this->editorCam->onUpdate(dt);
  }

  void
  EditorLayer::onImGuiRender()
  {
    Logger* logs = Logger::getInstance();

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
    for (auto& window : this->windows)
      if (window->isOpen)
        window->onImGuiRender(window->isOpen, this->currentScene);

  	if (ImGui::BeginMainMenuBar())
  	{
    	if (ImGui::BeginMenu("File"))
    	{
       	if (ImGui::MenuItem(ICON_FA_FILE_O" New", "Ctrl+N"))
       	{
          auto storage = Renderer3D::getStorage();
          storage->currentEnvironment->unloadEnvironment();

          this->currentScene = createShared<Scene>();
       	}
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN_O" pen...", "Ctrl+O"))
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
      {
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Add"))
      {
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scripts"))
      {
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings"))
      {
        if (ImGui::BeginMenu("Menus"))
        {
          if (ImGui::BeginMenu("Scene Menu Settings"))
          {
            if (ImGui::MenuItem("Show Scene Graph"))
            {
              this->windows[0]->isOpen = true;
            }

            if (ImGui::MenuItem("Material Settings"))
            {
              this->windows[4]->isOpen = true;
            }

            ImGui::EndMenu();
          }

          if (ImGui::BeginMenu("Editor Menu Settings"))
          {
            if (ImGui::MenuItem("Show Content Browser"))
            {
              this->windows[5]->isOpen = true;
            }

            if (ImGui::MenuItem("Show Performance Stats Menu"))
            {
              this->showPerf = true;
            }

            if (ImGui::MenuItem("Show Camera Menu"))
            {
              this->windows[1]->isOpen = true;
            }

            if (ImGui::MenuItem("Show Shader Menu"))
            {
              this->windows[2]->isOpen = true;
            }

            ImGui::EndMenu();
          }

          if (ImGui::MenuItem("Show Renderer Settings"))
          {
            this->windows[6]->isOpen = true;
          }

          ImGui::EndMenu();
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help"))
      {
        ImGui::EndMenu();
      }
     	ImGui::EndMainMenuBar();
  	}

    // The log menu.
    ImGui::Begin("Application Logs", nullptr);
    {
      if (ImGui::Button("Clear Logs"))
        Logger::getInstance()->getLogs() = "";

      ImGui::BeginChild("LogText");
      {
        auto size = ImGui::GetWindowSize();
        ImGui::PushTextWrapPos(size.x);
        ImGui::Text(Logger::getInstance()->getLogs().c_str());
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

          static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
          static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

          Shared<Scene> tempScene = createShared<Scene>();
          if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
          {
            this->currentScene = tempScene;
            this->currentScene->getSaveFilepath() = this->dndScenePath;
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
        static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
        static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

        Shared<Scene> tempScene = createShared<Scene>();
        if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
        {
          this->currentScene = tempScene;
          this->currentScene->getSaveFilepath() = this->dndScenePath;
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
      static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
      static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

      Shared<Scene> tempScene = createShared<Scene>();
      if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
      {
        this->currentScene = tempScene;
        this->currentScene->getSaveFilepath() = this->dndScenePath;
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

    bool camStationary = this->editorCam->isStationary();
    bool lControlHeld = appWindow->isKeyPressed(GLFW_KEY_LEFT_CONTROL);
    bool lShiftHeld = appWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT);

    switch (keyCode)
    {
      case GLFW_KEY_N:
      {
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
        {
          auto storage = Renderer3D::getStorage();
          storage->currentEnvironment->unloadEnvironment();

          this->currentScene = createShared<Scene>();
          static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
          static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());
        }
        break;
      }
      case GLFW_KEY_O:
      {
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                       ".srn"));
          this->loadTarget = FileLoadTargets::TargetScene;
        }
        break;
      }
      case GLFW_KEY_S:
      {
        if (lControlHeld && lShiftHeld && keyEvent.getRepeatCount() == 0 && camStationary)
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                       ".srn"));
          this->saveTarget = FileSaveTargets::TargetScene;
        }
        else if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
        {
          if (this->currentScene->getSaveFilepath() != "")
          {
            std::string path =  this->currentScene->getSaveFilepath();
            std::string name = path.substr(path.find_last_of('/') + 1, path.find_last_of('.'));

            YAMLSerialization::serializeScene(this->currentScene, path, name);
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
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int mouseCode = mouseEvent.getButton();

    bool camStationary = this->editorCam->isStationary();
    bool lControlHeld = appWindow->isKeyPressed(GLFW_KEY_LEFT_CONTROL);
    bool lShiftHeld = appWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT);

    switch (mouseCode)
    {
      default:
      {
        break;
      }
    }
  }

  Entity
  EditorLayer::getSelectedEntity()
  {
    return static_cast<SceneGraphWindow*>(this->windows[0])->getSelectedEntity();
  }
}
