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

// Projection (perspective) matrix.
glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

// Window objects.
Camera* sceneCam = new Camera(1920/2, 1080/2, glm::vec3 {0.0f, 0.0f, 4.0f}, EDITOR);

// Abstracted OpenGL objects.
GLFWwindow *window;
VertexArray* vArray;
Shader* program;
Renderer* renderer = Renderer::getInstance();

// Mesh data.
Mesh objModel1, objModel2;

// Spotlight colour.
glm::vec3 spotLight(1.0f, 1.0f, 1.0f);

void init()
{
	// Initialize the renderer.
	renderer->init(GL_FILL);

	// Load the obj file(s).
	objModel1.loadOBJFile("./models/bunny.obj");
	objModel1.normalizeVertices();
	//objModel2.loadOBJFile("./models/bunny.obj");
	//objModel2.normalizeVertices();

	// Generate the vertex and index buffers, assign them to a vertex array.
	VertexBuffer* verBuff = new VertexBuffer(&(objModel1.getData()[0]),
																					 objModel1.getData().size() * sizeof(Vertex),
																					 DYNAMIC);
	IndexBuffer* indBuff = new IndexBuffer(&(objModel1.getIndices()[0]),
																				 objModel1.getIndices().size(), DYNAMIC);
	vArray = new VertexArray(verBuff);
	vArray->addIndexBuffer(indBuff);

	// Link the vertex position and normal data to the variables in the NEW shader
	// program.
	program = new Shader("./res/r_shaders/default.vs", "./res/r_shaders/lighting.fs");

	// Spotlight on top of the model.
	program->addUniformVector("light.position", glm::vec4(0.0f, 2.0f, 0.0f, 1.0f));
	program->addUniformVector("light.direction", glm::vec3(0.0f, 1.0f, 0.0f));
	program->addUniformFloat("light.intensity", 1.0f);
	program->addUniformFloat("light.cosTheta", cos(glm::radians(45.0f)));
	program->addUniformVector("light.colour", spotLight);

	// Camera light properties.
	program->addUniformVector("camera.colour", spotLight);
	program->addUniformFloat("camera.intensity", 1.0f);
	program->addUniformFloat("camera.cosTheta", cos(glm::radians(12.0f)));
	program->addUniformVector("camera.colour", spotLight);

	// Vertex attributes.
	program->addAtribute("vPosition", VEC4, GL_FALSE, sizeof(Vertex), 0);
	program->addAtribute("vNormal", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal));
}

void framebufferSizeCallback(GLFWwindow *window, int w, int h) {
	// Prevent a divide by zero when the window is too short.
	if (h == 0)
		h = 1;

	float ratio = 1.0f * w / h;

	glfwMakeContextCurrent(window);

	glViewport(0, 0, w, h);
	projection = glm::perspective(glm::radians(90.0f), ratio, 0.1f, 100.0f);
}

// Display function, called for each frame.
void display()
{
	renderer->clear();
	// Model, view and projection transforms.
	glm::mat4 model = glm::mat4(1.0);
	glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
	glm::mat4 modelViewPerspective = projection * (*sceneCam->getViewMatrix()) * model;

	program->addUniformMatrix("model", model, GL_FALSE);
	program->addUniformMatrix("mVP", modelViewPerspective, GL_FALSE);
	program->addUniformMatrix("normalMat", normal, GL_FALSE);

	// Camera lighting uniforms.
	program->addUniformVector("camera.position", sceneCam->getCamPos());
	program->addUniformVector("camera.direction", sceneCam->getCamFront());
	program->addUniformVector("light.colour", spotLight);

	renderer->draw(vArray, program);
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
	sceneCam->scrollAction(xoffset, yoffset);
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

	// Setup the camera.
	sceneCam->init(window);

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
	glViewport(0, 0, 512, 512);

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
		ImGui::Begin("Performance");
    ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
								1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Press P to swap between freeform and editor.");
		ImGui::ColorEdit3("Clear colour", (float*)&clearColour);
		ImGui::ColorEdit3("Spotlight Colour", (float*)&spotLight);
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
