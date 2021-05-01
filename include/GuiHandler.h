#pragma once

// Dear Imgui includes.
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imguibrowser/FileBrowser/ImGuiFileBrowser.h"

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Lighting.h"
#include "Buffers.h"
#include "Camera.h"

namespace SciRenderer
{
  class GuiHandler
  {
  public:
    GuiHandler(LightController* lights);
    ~GuiHandler() = default;

    void init(GLFWwindow *window);
    void shutDown();

    void drawGUI(FrameBuffer* frontBuffer, Camera* editorCamera, GLFWwindow* window);
  private:
    // Window flags for the various GUI elements.
    const ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus;

    const ImGuiWindowFlags logFlags = ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

    const ImGuiWindowFlags editorFlags = ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

    // Handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;

    // Functions for specific GUI components.
    void lightingMenu();
    void modelMenu();
    void loadObjMenu();
    void logMenu();

    // Members for the log menu.
    std::string logBuffer;

    // Which submenus  to display.
    bool usePBR;

    // Members for the lighting menu.
    LightController* currentLights;
    std::vector<std::string> uLightNames;
    std::vector<std::string> pLightNames;
    std::vector<std::string> sLightNames;
    const char* currentULName;
    const char* currentPLName;
    const char* currentSLName;
    UniformLight* selectedULight;
    PointLight* 	selectedPLight;
    SpotLight* 		selectedSLight;

    // Members for the model menu.

    // Members for the texture menu.
  };
}
