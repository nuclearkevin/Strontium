#include "Layers/EditorLayer.h"

// Project includes.
#include "Core/Application.h"
#include "Core/Logs.h"

namespace SciRenderer
{
  EditorLayer::EditorLayer()
    : Layer("Editor Layer")
    , logBuffer("")
  {

  }

  EditorLayer::~EditorLayer()
  {

  }

  void
  EditorLayer::onAttach()
  {
    this->showPerf = true;
    this->usePBR = true;
    this->drawIrrad = false;
    this->drawFilter = false;
    this->mapRes = 512;

    // Fetch the width and height of the window and create a floating point
    // framebuffer.
    glm::ivec2 wDims = Application::getInstance()->getWindow()->getSize();
    this->drawBuffer = new FrameBuffer((GLuint) wDims.x, (GLuint) wDims.y);

    // Fetch a default floating point FBO spec and attach it.
    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    this->drawBuffer->attachTexture2D(cSpec);
  	this->drawBuffer->attachRenderBuffer();

    // Load the shader and set the appropriate uniforms.
    this->program = new Shader("./res/shaders/mesh.vs",
  											 "./res/shaders/pbr/pbr.fs");
  	this->program->addUniformSampler2D("irradianceMap", 0);
  	this->program->addUniformSampler2D("reflectanceMap", 1);
  	this->program->addUniformSampler2D("brdfLookUp", 2);

    // Initialize the PBR skybox.
  	this->skybox = new EnvironmentMap("./res/shaders/pbr/pbrSkybox.vs",
  													          "./res/shaders/pbr/pbrSkybox.fs",
  															      "./res/models/cube.obj");
  	this->skybox->loadEquirectangularMap("./res/textures/hdr_environments/checkers.hdr");
  	this->skybox->equiToCubeMap();
  	this->skybox->precomputeIrradiance();
  	this->skybox->precomputeSpecular();

    // Load in a default model (bunny) as a test.
    this->model = new Mesh();
    this->model->loadOBJFile("./res/models/bunny.obj");
  	this->model->normalizeVertices();

    // Finally, the editor camera.
    this->editorCam = new Camera(1920 / 2, 1080 / 2, glm::vec3 {0.0f, 1.0f, 4.0f},
                                 EditorCameraType::Stationary);
    this->editorCam->init(90.0f, 1.0f, 0.1f, 30.0f);
  }

  void
  EditorLayer::onDetach()
  {
    delete this->drawBuffer;
    delete this->program;
    delete this->skybox;
    delete this->editorCam;
    delete this->model;
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
			ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		style.WindowMinSize.x = minWinSizeX;

    bool openObjMenu = false;
    bool openEnvironment = false;

  	if (ImGui::BeginMainMenuBar())
  	{
    	if (ImGui::BeginMenu("File"))
    	{
       	if (ImGui::MenuItem("New Scene"))
       	{

       	}
        if (ImGui::MenuItem("Load Scene"))
       	{

       	}
        if (ImGui::MenuItem("Save Scene"))
       	{

       	}
        if (ImGui::MenuItem("Exit"))
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
        if (ImGui::MenuItem("Show Performance"))
        {
          this->showPerf = true;
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help"))
      {
        ImGui::EndMenu();
      }
     	ImGui::EndMainMenuBar();
  	}

    // The editor viewport.
    ImGui::Begin("Editor Viewport", nullptr, this->editorFlags);
    {
      ImGui::BeginChild("EditorRender");
        {
          ImVec2 editorSize = ImGui::GetWindowSize();
          ImGui::Image((ImTextureID) this->drawBuffer->getAttachID(FBOTargetParam::Colour0), editorSize,
                       ImVec2(0, 1), ImVec2(1, 0));
        }
      ImGui::EndChild();
    }
    ImGui::End();

    // The log menu.
    this->logBuffer += logs->getLastMessages();
    ImGui::Begin("Application Logs", nullptr, this->logFlags);
    {
      if (ImGui::Button("Clear Logs"))
        this->logBuffer = "";
      ImGui::BeginChild("LogText");
      {
        ImGui::Text(this->logBuffer.c_str());
      }
      ImGui::EndChild();
    }
    ImGui::End();

    // Left sidebar.
  	ImGui::Begin("Left Sidebar", nullptr, this->sidebarFlags);
    {
      // Left sidebar contents.
      if (ImGui::CollapsingHeader("Scene Graph"))
      {

      }

      // This should belong in the scene panel. TODO: Move it when scenes +
      // entities + components are done.
      if (ImGui::CollapsingHeader("Environment Map"))
      {
        if (ImGui::Button("Load Equirectangular Map"))
        {
          openEnvironment = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Environment Map"))
        {
          this->skybox->unloadEnvironment();
          this->drawIrrad = false;
          this->drawFilter = false;
          this->skybox->setDrawingType(MapType::Skybox);
        }

        if (this->skybox->hasEqrMap() && !this->skybox->hasSkybox())
        {
          ImGui::Text("Preview:");
          ImGui::Image((ImTextureID) this->skybox->getTexID(MapType::Equirectangular),
                       ImVec2(360, 180), ImVec2(0, 1), ImVec2(1, 0));
          ImGui::SliderInt("Environment Resolution", &this->mapRes, 512, 2048);
          if (ImGui::Button("Generate Skybox"))
          {
            this->skybox->equiToCubeMap(true, this->mapRes, this->mapRes);
          }
        }

        if (this->skybox->hasSkybox())
        {
          ImGui::SliderFloat("Gamma", &this->skybox->getGamma(), 1.0f, 5.0f);
          ImGui::SliderFloat("Exposure", &this->skybox->getExposure(), 1.0f, 5.0f);

          if (!this->skybox->hasIrradiance())
          {
            if (ImGui::Button("Generate Irradiance Map"))
            {
              this->skybox->precomputeIrradiance(256, 256, true);
            }
          }
          else
          {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Generate Irradiance Map");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();

            ImGui::Checkbox("Draw Irradiance Map", &this->drawIrrad);
            if (this->drawIrrad && !this->skybox->drawingFilter())
            {
              this->skybox->setDrawingType(MapType::Irradiance);
            }
            else if (!this->drawIrrad && !skybox->drawingFilter())
            {
              this->skybox->setDrawingType(MapType::Skybox);
            }
            else
            {
              this->drawIrrad = false;
            }
          }

          if (!this->skybox->hasPrefilter())
          {
            if (ImGui::Button("Generate BRDF Specular Map"))
            {
              this->skybox->precomputeSpecular(this->mapRes, this->mapRes);
            }
          }
          else
          {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Generate BRDF Specular Map");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();

            ImGui::Checkbox("Draw Pre-Filter Map", &this->drawFilter);
            if (this->drawFilter && !this->skybox->drawingIrrad())
            {
              this->skybox->setDrawingType(MapType::Prefilter);
              ImGui::SliderFloat("Roughness", &this->skybox->getRoughness(), 0.0f, 1.0f);
            }
            else if (!this->drawFilter && !skybox->drawingIrrad())
            {
              this->skybox->setDrawingType(MapType::Skybox);
            }
            else
            {
              this->drawFilter = false;
            }
          }

          if (skybox->hasIntegration())
          {
            ImGui::Text("BRDF Lookup Texture:");
            ImGui::Image((ImTextureID) this->skybox->getTexID(MapType::Integration),
                         ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
          }
        }
      }
    }
    ImGui::End();

    // The right sidebar.
  	ImGui::Begin("Right Sidebar", nullptr, this->sidebarFlags);
    {
      // Right sidebar contents.
    }
    ImGui::End();

    // The performance window.
    if (this->showPerf)
    {
      ImGui::Begin("Performance Window", &this->showPerf);
      ImGui::Text(Application::getInstance()->getWindow()->getContextInfo().c_str());
      ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // Open a file browser to load .hdr files.
    if (openEnvironment)
      ImGui::OpenPopup("Load Equirectangular Map");

    if (this->fileHandler.showFileDialog("Load Equirectangular Map",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".hdr"))
    {
      this->skybox->loadEquirectangularMap(this->fileHandler.selected_path);
    }

    // MUST KEEP THIS. Docking window end.
    ImGui::End();
  }

  // On event for the laryer.
  void
  EditorLayer::onEvent(Event &event)
  {
    this->editorCam->onEvent(event);
  }

  // On update for the layer.
  void
  EditorLayer::onUpdate(float dt)
  {
    //--------------------------------------------------------------------------
    // The draw loop.
    //--------------------------------------------------------------------------
    // Get the renderer.
    Renderer3D* renderer = Renderer3D::getInstance();

    // Draw the scene.
    this->drawBuffer->clear();
  	this->drawBuffer->bind();
  	this->drawBuffer->setViewport();

    this->skybox->bind(MapType::Irradiance, 0);
  	this->skybox->bind(MapType::Prefilter, 1);
  	this->skybox->bind(MapType::Integration, 2);
  	renderer->draw(this->model, this->program, this->editorCam);

    this->skybox->draw(this->editorCam);
  	this->drawBuffer->unbind();

    //--------------------------------------------------------------------------
    // Update the editor camera.
    //--------------------------------------------------------------------------
    this->editorCam->onUpdate(dt);
  }
}
