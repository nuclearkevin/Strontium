// Include macro file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Shaders.h"
#include "Meshes.h"
#include "Camera.h"
#include "Renderer.h"
#include "Lighting.h"

// Dear Imgui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

using namespace SciRenderer;

// Window objects.
Camera* sceneCam;

// Abstracted OpenGL objects.
GLFWwindow *window;
Shader* program;
LightController* lights;
Renderer* renderer = Renderer::getInstance();

// Mesh data.
Mesh objModel1, objModel2;

glm::vec4 clearColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

void init()
{
	// Initialize the renderer.
	renderer->init(GL_FILL);

	// Initialize the lighting system.
	lights = new LightController("./res/r_shaders/lights.vs",
															 "./res/r_shaders/lights.fs",
															 "./models/sphere.obj");
	program = new Shader("./res/r_shaders/default.vs", "./res/r_shaders/lighting.fs");

	// Initialize the camera.
	sceneCam = new Camera(1920/2, 1080/2, glm::vec3 {0.0f, 0.0f, 4.0f}, EDITOR);
	sceneCam->init(window, glm::perspective(90.0f, 1.0f, 0.1f, 100.0f));

	// Load the obj file(s).
	objModel1.loadOBJFile("./models/bunny.obj");
	objModel1.normalizeVertices();
	objModel2.loadOBJFile("./models/teapot.obj");
	objModel2.normalizeVertices();

	// Setup a single spotlight under the teapot and a uniform light from the top.
	SpotLight light1;
	light1.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	light1.direction = glm::vec3(0.0f, 1.0f, 0.0f);
	light1.position = glm::vec4(0.0f, 2.0f, 0.0f, 1.0f);
	light1.intensity = 1.0f;
	light1.innerCutOff = cos(glm::radians(45.0f));
	light1.outerCutOff = cos(glm::radians(65.0f));
	lights->addLight(light1, 0.1f);
	light1.colour = glm::vec3(1.0f, 0.0f, 0.0f);
	light1.direction = glm::vec3(0.0f, -1.0f, 0.0f);
	light1.position = glm::vec4(0.0f, -2.0f, 0.0f, 1.0f);
	lights->addLight(light1, 0.1f);
}

// Display function, called for each frame.
void display()
{
	// Clear the frame buffer and depth buffer.
	renderer->clear();

	// Draw the light meshes.
	lights->drawLightMeshes(renderer, sceneCam);
	// Prepare the scene lighting.
	lights->setLighting(program);
	// Draw the bunny.
	//renderer->draw(&objModel1, program, sceneCam);
	// Draw the teapot.
	renderer->draw(&objModel2, program, sceneCam);
}

// Error callback, called when there is an error during program initialization
// or runtime.
void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

// Size callback, resizes the window when required.
void framebufferSizeCallback(GLFWwindow *window, int w, int h)
{
	if (h == 0)
		h = 1;
	float ratio = 1.0f * w / h;

	glfwMakeContextCurrent(window);
	glViewport(0, 0, w, h);
	sceneCam->updateProj(90.0f, ratio, 0.1f, 100.0f);
}

// Key callback, called each time a key is pressed.
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		sceneCam->swap(window);
}

// Scroll callback.
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	sceneCam->scrollAction(window, xoffset, yoffset);
}

int main(int argc, char **argv) {
	// Set the error callback so we get output when things go wrong.
	glfwSetErrorCallback(error_callback);

	// Initialize GLFW.
	if (!glfwInit())
		fprintf(stderr, "Can't initialize GLFW\n");

	// Open the window using GLFW.
	window = glfwCreateWindow(1920, 1080, "SciRender", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Set the remaining callbacks.
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Initialize GLEW.
	glfwMakeContextCurrent(window);
	GLenum error = glewInit();
	if (error != GLEW_OK) {
		printf("Error starting GLEW: %s\n",glewGetErrorString(error));
		exit(0);
	}

	// Dear Imgui init.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 440");

	glViewport(0, 0, 1920, 1080);

	init();

	glfwSwapInterval(1);

	// GLFW main loop, display model, swapbuffer and check for input.
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		sceneCam->mouseAction(window);
		sceneCam->keyboardAction(window);

		ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

		display();

		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
		ImGui::Begin("Performance and Settings:");
    ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
								1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Press P to swap between freeform and editor.");
		ImGui::ColorEdit3("Clear colour", (float*)&clearColour);
    ImGui::End();

		ImGui::Render();
		glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		renderer->swap(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup pointers.
	delete program;
	delete renderer;
	glfwTerminate();
}
