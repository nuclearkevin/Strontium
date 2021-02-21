#include "GuiHandler.h"

// Dear Imgui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

using namespace SciRenderer;

GuiHandler::GuiHandler(LightController* lights)
  : lighting(false)
  , model(false)
  , currentLights(lights)
{
  this->currentULName = "----";
  this->currentPLName = "----";
  this->currentSLName = "----";
  this->selectedULight = nullptr;
  this->selectedPLight = nullptr;
  this->selectedSLight = nullptr;
}

GuiHandler::~GuiHandler() { }

void
GuiHandler::init(GLFWwindow *window)
{
  // Dear Imgui init.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 440");
}

void
GuiHandler::shutDown()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void
GuiHandler::drawGUI()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

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
     	ImGui::EndMenu();
   	}
    if (ImGui::BeginMenu("Models"))
  	{
      if (ImGui::MenuItem("Load Model"))
     	{

     	}
      if (ImGui::MenuItem("Load Texture"))
     	{

     	}
      if (ImGui::MenuItem("Load Material"))
     	{

     	}
     	ImGui::EndMenu();
    }
		if (ImGui::BeginMenu("Scene Settings"))
  	{
     	if (ImGui::MenuItem("Lights"))
      {
     	  this->lighting = true;
      }
      if (ImGui::MenuItem("Models"))
      {

      }
     	ImGui::EndMenu();
   	}

   	ImGui::EndMainMenuBar();
	}

	// The performance stats window.
	ImGui::SetNextWindowPos(ImVec2(0, 20));
	ImGui::SetNextWindowSize(ImVec2(450, 75), ImGuiCond_Always);
	ImGui::Begin("Performance");
	ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
							1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Press P to swap between freeform and editor");
	ImGui::End();

  // Other windows of note.
  if (this->lighting)
    this->lightingMenu();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
GuiHandler::lightingMenu()
{
  // Get the gui labels for the lights.
  this->uLightNames = this->currentLights->getGuiLabel(UNIFORM);
  this->pLightNames = this->currentLights->getGuiLabel(POINT);
  this->sLightNames = this->currentLights->getGuiLabel(SPOT);

  // The lighting window.
	ImGui::SetNextWindowPos(ImVec2(0, 95));
	ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_Always);
	ImGui::Begin("Lights", &this->lighting);
	ImGui::Text("Total of %d lightcasters", this->currentLights->getNumLights(ALL));
	ImGui::ColorEdit3("Ambient colour", &this->currentLights->getAmbient()->x);
	// Dropdown box for uniform lights.
	if (ImGui::BeginCombo("Uniform lights", this->currentULName))
	{
		for (unsigned i = 0; i < this->uLightNames.size(); i++)
		{
			bool isLightSelected = (this->currentULName == this->uLightNames[i].c_str());
			if (ImGui::Selectable(this->uLightNames[i].c_str(), isLightSelected))
			{
					this->currentULName = this->uLightNames[i].c_str();
					if (i - 1 < 0)
						this->selectedULight = nullptr;
					else
						this->selectedULight = this->currentLights->getULight(i - 1);
			}
			if (isLightSelected)
					ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	// Menu to modify the uniform light properties.
	if (this->selectedULight != nullptr)
	{
		ImGui::SliderFloat3("  U-Light direction", &(this->selectedULight->direction.x), -1.0f, 1.0f);
		ImGui::ColorEdit3("  U-Light colour", &(this->selectedULight->colour.x));
		ImGui::SliderFloat("  U-Light intensity", &(this->selectedULight->intensity), 0.0f, 1.0f);
		if (ImGui::Button((std::string("Delete ") + this->selectedULight->name).c_str()))
		{
			this->currentLights->deleteLight(UNIFORM, this->selectedULight->lightID);
			this->selectedULight = nullptr;
			this->currentULName = "----";
			this->currentLights->setGuiLabel(UNIFORM);
		}
	}
	// Dropdown box for point lights.
	if (ImGui::BeginCombo("Point lights", this->currentPLName))
	{
		for (unsigned i = 0; i < this->pLightNames.size(); i++)
		{
			bool isLightSelected = (this->currentPLName == this->pLightNames[i].c_str());
			if (ImGui::Selectable(this->pLightNames[i].c_str(), isLightSelected))
			{
					this->currentPLName = this->pLightNames[i].c_str();
					if (i - 1 < 0)
						this->selectedPLight = nullptr;
					else
						this->selectedPLight = this->currentLights->getPLight(i - 1);
			}
			if (isLightSelected)
					ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	// Menu to modify point light properties.
	if (this->selectedPLight != nullptr)
	{
		ImGui::SliderFloat3("  P-Light position", &(this->selectedPLight->position.x), -10.0f, 10.0f);
		ImGui::ColorEdit3("  P-Light colour", &(this->selectedPLight->colour.x));
		ImGui::SliderFloat("  P-Light intensity", &(this->selectedPLight->intensity), 0.0f, 1.0f);
		ImGui::SliderFloat("  P-Light mesh scale", &(this->selectedPLight->meshScale), 0.0f, 1.0f);
		if (ImGui::Button((std::string("Delete ") + this->selectedPLight->name).c_str()))
		{
			this->currentLights->deleteLight(POINT, this->selectedPLight->lightID);
			this->selectedPLight = nullptr;
			this->currentPLName = "----";
			this->currentLights->setGuiLabel(POINT);
		}
	}
	// Dropdown box for spot lights.
	if (ImGui::BeginCombo("Spotlights", this->currentSLName))
	{
		for (unsigned i = 0; i < this->sLightNames.size(); i++)
		{
			bool isLightSelected = (this->currentSLName == this->sLightNames[i].c_str());
			if (ImGui::Selectable(this->sLightNames[i].c_str(), isLightSelected))
			{
					this->currentSLName = this->sLightNames[i].c_str();
					if (i - 1 < 0)
						this->selectedSLight = nullptr;
					else
						this->selectedSLight = this->currentLights->getSLight(i - 1);
			}
			if (isLightSelected)
					ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	// Menu to modify spot light properties.
	if (selectedSLight != nullptr)
	{
		ImGui::SliderFloat3("  S-Light position", &(this->selectedSLight->position.x), -10.0f, 10.0f);
		ImGui::SliderFloat3("  S-Light direction", &(this->selectedSLight->direction.x), -1.0f, 1.0f);
		ImGui::ColorEdit3("  S-Light colour", &(this->selectedSLight->colour.x));
		ImGui::SliderFloat("  S-Light intensity", &(this->selectedSLight->intensity), 0.0f, 1.0f);
		ImGui::SliderFloat("  S-Light outer cutoff", &(this->selectedSLight->innerCutOff), 0.0f, 1.0f);
		ImGui::SliderFloat("  S-Light inner cutoff", &(this->selectedSLight->outerCutOff), 0.0f, 1.0f);
		ImGui::SliderFloat("  S-Light mesh scale", &(this->selectedSLight->meshScale), 0.0f, 1.0f);
		if (ImGui::Button((std::string("Delete ") + this->selectedSLight->name).c_str()))
		{
			this->currentLights->deleteLight(SPOT, this->selectedSLight->lightID);
			this->selectedSLight = nullptr;
			this->currentSLName = "----";
			this->currentLights->setGuiLabel(SPOT);
		}
	}
	// Buttons to create lights.
	if (ImGui::Button("New uniform light"))
	{
		UniformLight temp;
		temp.colour = glm::vec3(0.0f, 0.0f, 0.0f);
		temp.direction = glm::vec3(0.0f, 0.0f, 0.0f);
		temp.intensity = 0.0f;
		this->currentLights->addLight(temp);
	}
	ImGui::SameLine();
	if (ImGui::Button("New point light"))
	{
		PointLight temp;
		temp.colour = glm::vec3(0.0f, 0.0f, 0.0f);
		temp.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		temp.intensity = 0.0f;
		this->currentLights->addLight(temp, 0.1f);
	}
	ImGui::SameLine();
	if (ImGui::Button("New spotlight"))
	{
		SpotLight temp;
		temp.colour = glm::vec3(0.0f, 0.0f, 0.0f);
		temp.direction = glm::vec3(0.0f, 0.0f, 0.0f);
		temp.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		temp.intensity = 0.0f;
		temp.innerCutOff = 0.0f;
		temp.outerCutOff = 0.0f;
		this->currentLights->addLight(temp, 0.1f);
	}
	ImGui::End();
}

void
GuiHandler::modelMenu()
{

}
