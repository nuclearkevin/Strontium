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

    // Set ImGui to dark mode, because why would we not?
    ImGui::StyleColorsDark();

    // Fetch the application window.
    Application* app = Application::getInstance();
    GLFWwindow* window = app->getWindow()->getWindowPtr();

    // Setup the ImGui main window.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
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
  }
}
