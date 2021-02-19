// Include macro file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Shaders.h"
#include "Meshes.h"
#include "Buffers.h"
#include "VertexArray.h"
#include "Camera.h"
#include "Renderer.h"

// Dear Imgui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

using namespace SciRenderer;

// Window objects.
Camera* sceneCam;

// Abstracted OpenGL objects.
GLFWwindow *window;
VertexArray* vArray;
Shader* program;
Renderer* renderer = Renderer::getInstance();

// Mesh data.
Mesh objModel1, objModel2;

// Spotlight colour.
glm::vec3 spotLight(1.0f, 1.0f, 1.0f);
glm::vec4 spotLightPos(0.0f, 2.0f, 0.0f, 1.0f);

void init()
{
	// Initialize the renderer.
	renderer->init(GL_FILL);

	// Initialize the camera.
	sceneCam = new Camera(1920/2, 1080/2, glm::vec3 {0.0f, 0.0f, 4.0f}, EDITOR);
	sceneCam->init(window, glm::perspective(90.0f, 1.0f, 0.1f, 100.0f));

	// Load the obj file(s).
	objModel1.loadOBJFile("./models/bunny.obj");
	objModel1.normalizeVertices();
	objModel2.loadOBJFile("./models/teapot.obj");
	objModel2.normalizeVertices();

	// Link the vertex position and normal data to the variables in the NEW shader
	// program.
	program = new Shader("./res/r_shaders/default.vs", "./res/r_shaders/lighting.fs");

	// Spotlight on top of the model.
	program->addUniformVector("light.direction", glm::vec3(0.0f, 1.0f, 0.0f));
	program->addUniformFloat("light.intensity", 1.0f);
	program->addUniformFloat("light.cosTheta", cos(glm::radians(45.0f)));
	program->addUniformVector("light.colour", spotLight);

	// Camera light properties.
	program->addUniformVector("camera.colour", spotLight);
	program->addUniformFloat("camera.intensity", 1.0f);
	program->addUniformFloat("camera.cosTheta", cos(glm::radians(12.0f)));
	program->addUniformFloat("camera.cosGamma", cos(glm::radians(24.0f)));
	program->addUniformVector("camera.colour", spotLight);
}
void framebufferSizeCallback(GLFWwindow *window, int w, int h) {
	// Prevent a divide by zero when the window is too short.
	if (h == 0)
		h = 1;
	float ratio = 1.0f * w / h;

	glfwMakeContextCurrent(window);
	glViewport(0, 0, w, h);
	sceneCam->updateProj(90.0f, ratio, 0.1f, 100.0f);
}

// Display function, called for each frame.
void display()
{
	// Clear the framebuffer and depthbuffer.
	renderer->clear();

	// Camera lighting uniforms.
	program->addUniformVector("camera.position", sceneCam->getCamPos());
	program->addUniformVector("camera.direction", sceneCam->getCamFront());
	program->addUniformVector("light.colour", spotLight);
	program->addUniformVector("light.position", spotLightPos);

	// Draw the bunny.
	renderer->draw(&objModel1, program, sceneCam);
	// Draw the teapot.
	renderer->draw(&objModel2, program, sceneCam);
}

// Key callback, called each time a key is pressed.
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		sceneCam->swap(window);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	sceneCam->scrollAction(window, xoffset, yoffset);
}

// Error callback, called when there is an error during program initialization
// or runtime.
void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
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

	ImVec4 clearColour = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
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
		ImGui::ColorEdit3("Spotlight Colour", (float*)&spotLight);
		ImGui::SliderFloat("Splotlight x-position", &spotLightPos.x, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Splotlight y-position", &spotLightPos.y, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Splotlight z-position", &spotLightPos.z, -10.0f, 10.0f, "%.1f");
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
	delete vArray;
	delete program;
	delete renderer;
	glfwTerminate();
}
