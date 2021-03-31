#pragma once

// Dear Imgui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/ImGuiFileBrowser.h"

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Lighting.h"

namespace SciRenderer
{
  class GuiHandler
  {
  public:
    GuiHandler(LightController* lights);
    ~GuiHandler();

    void init(GLFWwindow *window);
    void shutDown();

    void drawGUI();
  private:
    // Handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;

    // Functions for specific menus.
    void lightingMenu();
    void modelMenu();
    void loadObjMenu();

    // Menus to draw at any one moment.
    bool lighting;
    bool model;

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
