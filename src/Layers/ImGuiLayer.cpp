#include "Layers/ImGuiLayer.h"

// Project includes.
#include "Core/Application.h"

namespace SciRenderer
{
  ImGuiLayer::ImGuiLayer()
    : Layer("ImGui Layer")
  { }

  void
  ImGuiLayer::onAttach()
  {
    // ImGui startup boilerplate.
    IMGUI_CHECKVERSION();
		ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDragClickToInputText = true;

    io.FontDefault = io.Fonts->AddFontFromFileTTF("./assets/.fonts/Roboto/Roboto-Black.ttf", 14.0f);
    io.Fonts->AddFontFromFileTTF("./assets/.fonts/Roboto/Roboto-Bold.ttf", 14.0f);

    // Set ImGui to dark mode, because why would we not?
    ImGui::StyleColorsDark();

    // Fetch the application window.
    Application* app = Application::getInstance();

    // Setup the ImGui main window.
    ImGui_ImplGlfw_InitForOpenGL(app->getWindow()->getWindowPtr(), true);
    ImGui_ImplOpenGL3_Init("#version 440");
  }

  void
  ImGuiLayer::onDetach()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
  	ImGui::DestroyContext();
  }

  void
  ImGuiLayer::onEvent(Event &event)
  {

  }

  void
  ImGuiLayer::beginImGui()
  {
    ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
  }

  void
  ImGuiLayer::endImGui()
  {
    // Update the size of the ImGui display.
    ImGuiIO& io = ImGui::GetIO();
		Application* app = Application::getInstance();
    glm::ivec2 windowSize = app->getWindow()->getSize();
		io.DisplaySize = ImVec2((float) windowSize.x, (float) windowSize.y);

    ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Viewport handling. Backup the context to make sure it doesn't break.
    GLFWwindow* backupContext = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backupContext);
  }
}
