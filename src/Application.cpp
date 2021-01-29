// Include macro file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Shaders.h"
#include "Meshes.h"
#include "Buffers.h"
#include "VertexArray.h"
#include "Camera.h"
#include "Renderer.h"

// Projection (perspective) matrix.
glm::mat4 projection;

// Window objects.
Camera* sceneCam = new Camera(512/2, 512/2, glm::vec3 {0.0f, 0.0f, 4.0f});

// Mesh data.
Mesh objModel1, objModel2;

// Abstracted OpenGL objects.
GLFWwindow *window;
VertexArray* vArray;
Shader* program;
Renderer* renderer = Renderer::getInstance();

void init()
{
	// Initialize the renderer.
	renderer->init(GL_FILL);

	// Load the obj file(s).
	objModel2.loadOBJFile("./models/teapot.obj");
	objModel2.normalizeVertices();
	objModel1.loadOBJFile("./models/bunny.obj");
	objModel1.normalizeVertices();

	// Generate the vertex and index buffers, assign them to a vertex array.
	VertexBuffer* verBuff = buildBatchVBuffer(std::vector<Mesh*>
																					  { &objModel1, &objModel2 }, DYNAMIC);
	IndexBuffer* indBuff = buildBatchIBuffer(std::vector<Mesh*>
																					 { &objModel1, &objModel2 }, DYNAMIC);
	vArray = new VertexArray(verBuff);
	vArray->addIndexBuffer(indBuff);

	// Link the vertex position and normal data to the variables in the NEW shader
	// program.
	program = new Shader("./res/shaders/default.vs", "./res/shaders/lighting.fs");

	// Spotlight on top of the model.
	program->addUniformVector("light.position", glm::vec4(0.0f, 2.0f, 0.0f, 1.0f));
	program->addUniformVector("light.direction", glm::vec3(0.0f, 1.0f, 0.0f));
	program->addUniformVector("light.colour", glm::vec3(1.0f, 1.0f, 1.0f));
	program->addUniformFloat("light.intensity", 1.0f);
	program->addUniformFloat("light.cosTheta", cos(glm::radians(45.0f)));

	// Camera light properties.
	program->addUniformVector("camera.colour", glm::vec3(1.0f, 1.0f, 1.0f));
	program->addUniformFloat("camera.intensity", 1.0f);
	program->addUniformFloat("camera.cosTheta", cos(glm::radians(12.0f)));

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
	projection = glm::perspective(0.7f, ratio, 1.0f, 100.0f);
}

// Display function, called for each frame.
void display()
{
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

	renderer->draw(window, vArray, program);
}

// Key callback, called each time a key is pressed.
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
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
	window = glfwCreateWindow(512, 512, "SciRender", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Set the remaining callbacks.
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, key_callback);

	// Capture the cursor.
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize GLEW.
	glfwMakeContextCurrent(window);
	GLenum error = glewInit();
	if (error != GLEW_OK) {
		printf("Error starting GLEW: %s\n",glewGetErrorString(error));
		exit(0);
	}

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glViewport(0, 0, 512, 512);

	projection = glm::perspective(0.7f, 1.0f, 1.0f, 100.0f);

	init();

	glfwSwapInterval(1);

	// GLFW main loop, display model, swapbuffer and check for input.
	while (!glfwWindowShouldClose(window))
	{
		sceneCam->mouseAction(window);
		sceneCam->keyboardAction(window);
		display();
		glfwPollEvents();
	}

	// Cleanup pointers.
	delete vArray;
	delete program;
	delete renderer;
	glfwTerminate();
}
