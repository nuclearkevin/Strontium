// Include macro file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/Logs.h"
#include "Graphics/Shaders.h"
#include "Graphics/Meshes.h"
#include "Graphics/Camera.h"
#include "Graphics/Renderer.h"
#include "Graphics/Lighting.h"
#include "Graphics/GuiHandler.h"
#include "Graphics/Textures.h"
#include "Graphics/EnvironmentMap.h"

using namespace SciRenderer;

// Window size at launch.
GLuint width = 1920;
GLuint height = 1080;

// Window objects.
Camera* sceneCam;
GLFWwindow* window;

// Abstracted OpenGL objects.
Logger* 						 logs = Logger::getInstance();
FrameBuffer* 		 		 drawBuffer;
Shader* 				 		 program;
Shader* 				 		 vpPassthrough;
VertexArray* 		 		 viewport;
LightController* 		 lights;
GuiHandler* 		 		 frontend;
Renderer3D* 			 	 renderer = Renderer3D::getInstance();
EnvironmentMap*      skybox;

void init()
{
	logs->init();
	// Initialize the framebuffer for drawing.
	drawBuffer = new FrameBuffer(width, height);
	drawBuffer->generateColourTexture2D();
	drawBuffer->generateRenderBuffer();

	// Initialize the renderer.
	renderer->init("./res/shaders/viewport.vs", "./res/shaders/viewport.fs");

	// Initialize the lighting system.
	lights = new LightController("./res/shaders/lightMesh.vs",
															 "./res/shaders/lightMesh.fs",
															 "./res/models/sphere.obj");
	program = new Shader("./res/shaders/mesh.vs",
											 "./res/shaders/phong/spec.fs");

	// Initialize the camera.
	sceneCam = new Camera(width / 2, height / 2, glm::vec3 {0.0f, 1.0f, 4.0f}, EDITOR);
	sceneCam->init(window, glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 30.0f));

	// Initialize the PBR skybox.
	skybox = new EnvironmentMap("./res/shaders/pbr/pbrSkybox.vs",
													    "./res/shaders/pbr/pbrSkybox.fs",
															"./res/models/cube.obj");
	skybox->loadEquirectangularMap("./res/textures/hdr_environments/Factory_Catwalk_2k.hdr", {}, true, 2048, 2048);

	// Setup a basic uniform light field.
	UniformLight light1;
	light1.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	light1.direction = glm::vec3(0.0f, -1.0f, 0.0f);
	light1.intensity = 0.5f;
	light1.mat.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	light1.mat.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	light1.mat.attenuation = glm::vec2(0.0f, 0.0f);
	light1.mat.shininess = 64.0f;
	lights->addLight(light1);
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

	// Draw the skybox.
	skybox->draw(sceneCam);
	drawBuffer->unbind();
}

// Error callback, called when there is an error during program initialization
// or runtime.
void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

// Key callback, called each time a key is pressed.
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		sceneCam->swap(window);
}

// Scroll callback.
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	sceneCam->scrollAction(window, xoffset, yoffset);
}

int main(int argc, char **argv)
{
	// Set the error callback so we get output when things go wrong.
	glfwSetErrorCallback(error_callback);

	// Initialize GLFW.
	if (!glfwInit())
	{
		fprintf(stderr, "Can't initialize GLFW\n");
	}

	// Open the window using GLFW.
	window = glfwCreateWindow(width, height, "Editor Window", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Initialize GLAD using the GLFW instance.
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
	{
		std::cout << "Error initializing GLAD!" << std::endl;
		exit(0);
	}

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, width, height);

	init();
	frontend = new GuiHandler(lights);
	frontend->init(window);

	glfwSwapInterval(1);

	// Set the remaining callbacks.
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Main application loop.
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		sceneCam->mouseAction(window);
		sceneCam->keyboardAction(window);

		display();

		frontend->drawGUI(drawBuffer, sceneCam, skybox, window);
		renderer->swap(window);
	}

	frontend->shutDown();

	// Cleanup pointers.
	delete program;
	delete vpPassthrough;
	delete lights;
	delete frontend;
	delete drawBuffer;
	delete renderer;
	delete skybox;

	glfwTerminate();
}
