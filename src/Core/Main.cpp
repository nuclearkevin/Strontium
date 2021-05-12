// Include macro file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/Window.h"
#include "Core/Events.h"
#include "Graphics/Shaders.h"
#include "Graphics/Meshes.h"
#include "Graphics/Camera.h"
#include "Graphics/Renderer.h"
#include "Graphics/Lighting.h"
#include "Graphics/GuiHandler.h"
#include "Graphics/Textures.h"
#include "Graphics/EnvironmentMap.h"
#include "Graphics/FrameBuffer.h"

using namespace SciRenderer;

// Window size at launch.
GLuint width = 1920;
GLuint height = 1080;

// Window objects.
Camera* sceneCam;
GLFWwindow* window;
Window* myWindow;

// Abstracted OpenGL objects.
Logger* 						 logs = Logger::getInstance();
EventDispatcher*     appEvents = EventDispatcher::getInstance();
FrameBuffer* 		 		 drawBuffer;
Shader* 				 		 program;
Shader* 				 		 vpPassthrough;
LightController* 		 lights;
GuiHandler* 		 		 frontend;
Renderer3D* 			 	 renderer;
EnvironmentMap*      skybox;

Mesh bunny;

void init()
{
	renderer = Renderer3D::getInstance();
	renderer->init("./res/shaders/viewport.vs", "./res/shaders/viewport.fs");

	// Initialize the framebuffer for drawing.
	drawBuffer = new FrameBuffer(width, height);

	// Attach a colour texture at binding 0 and a renderbuffer.
	auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
	drawBuffer->attachTexture2D(cSpec);
	drawBuffer->attachRenderBuffer();

	// Initialize the lighting system.
	lights = new LightController("./res/shaders/lightMesh.vs",
															 "./res/shaders/lightMesh.fs",
															 "./res/models/sphere.obj");
	program = new Shader("./res/shaders/mesh.vs",
											 "./res/shaders/pbr/pbr.fs");
	program->addUniformSampler2D("irradianceMap", 0);
	program->addUniformSampler2D("reflectanceMap", 1);
	program->addUniformSampler2D("brdfLookUp", 2);

	// Initialize the camera.
	sceneCam = new Camera(width / 2, height / 2, glm::vec3 {0.0f, 1.0f, 4.0f}, EDITOR);
	sceneCam->init(window, glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 30.0f));

	// Initialize the PBR skybox.
	skybox = new EnvironmentMap("./res/shaders/pbr/pbrSkybox.vs",
													    "./res/shaders/pbr/pbrSkybox.fs",
															"./res/models/cube.obj");
	skybox->loadEquirectangularMap("./res/textures/hdr_environments/checkers.hdr");
	skybox->equiToCubeMap();
	skybox->precomputeIrradiance();
	skybox->precomputeSpecular();

	// Test object.
	bunny.loadOBJFile("./res/models/bunny.obj");
	bunny.normalizeVertices();
}

// Display function, called for each frame.
void display()
{
	// Clear and bind the framebuffer.
	drawBuffer->clear();
	drawBuffer->bind();
	drawBuffer->setViewport();

	// Prepare the scene lighting.
	lights->setLighting(program, sceneCam);

	// Draw the light meshes.
	lights->drawLightMeshes(sceneCam);

	skybox->bind(MapType::Irradiance, 0);
	skybox->bind(MapType::Prefilter, 1);
	skybox->bind(MapType::Integration, 2);
	renderer->draw(&bunny, program, sceneCam);

	// Draw the skybox.
	skybox->draw(sceneCam);
	drawBuffer->unbind();
}

int main(int argc, char **argv)
{
	myWindow = Window::getNewInstance("Editor Window", 1920, 1080);
	window = myWindow->getWindowPtr();

	init();
	frontend = new GuiHandler(lights);
	frontend->init(window);

	// Main application loop.
	while (!glfwWindowShouldClose(window))
	{
		myWindow->onUpdate();
		sceneCam->mouseAction(window);
		sceneCam->keyboardAction(window);

		display();

		frontend->drawGUI(drawBuffer, sceneCam, skybox, window);
		renderer->swap(window);
	}

	frontend->shutDown();

	// Cleanup pointers.
	delete appEvents;
	delete program;
	delete vpPassthrough;
	delete lights;
	delete frontend;
	delete drawBuffer;
	delete renderer;
	delete skybox;
	delete sceneCam;

	// Shut down the window.
	delete myWindow;
}
