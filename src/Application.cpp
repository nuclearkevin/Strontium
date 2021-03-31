// Include macro file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Shaders.h"
#include "Meshes.h"
#include "Camera.h"
#include "Renderer.h"
#include "Lighting.h"
#include "GuiHandler.h"
#include "Textures.h"

using namespace SciRenderer;

// Window size.
GLuint width = 800;
GLuint height = 800;

// Window objects.
Camera* sceneCam;
GLFWwindow* window;

// Abstracted OpenGL objects.
FrameBuffer* 		 		 drawBuffer;
Shader* 				 		 program;
Shader* 				 		 vpPassthrough;
Shader*					 		 PBR;
VertexArray* 		 		 viewport;
LightController* 		 lights;
GuiHandler* 		 		 gui;
Renderer* 			 		 renderer = Renderer::getInstance();
EnvironmentMap*      skybox;
Texture2DController* textures;

// Mesh data.
Mesh objModel1, objModel2, objModel3, ground;

GLuint framebuffer, textureColorbuffer, rbo;
GLuint quadVAO;
Shader* screenShader;

void init()
{
	// Initialize the framebuffer for drawing.
	drawBuffer = new FrameBuffer(width, height);
	drawBuffer->attachColourTexture2D();
	drawBuffer->attachRenderBuffer();

	// Initialize the renderer.
	renderer->init("./res/r_shaders/viewport.vs", "./res/r_shaders/viewport.fs");

	// Initialize the lighting system.
	lights = new LightController("./res/r_shaders/lightMesh.vs",
															 "./res/r_shaders/lightMesh.fs",
															 "./res/models/sphere.obj");
	program = new Shader("./res/r_shaders/mesh.vs",
											 "./res/r_shaders/phong/spec.fs");

	PBR = new Shader("./res/r_shaders/mesh.vs",
									 "./res/r_shaders/pbr/pbrTex.fs");
	PBR->addUniformSampler2D("albedoMap", 0);
	PBR->addUniformSampler2D("normalMap", 1);
	PBR->addUniformSampler2D("roughnessMap", 2);
	PBR->addUniformSampler2D("metallicMap", 3);
	PBR->addUniformSampler2D("aOcclusionMap", 4);
	PBR->unbind();

	// Initialize the camera.
	sceneCam = new Camera(width/2, height/2, glm::vec3 {0.0f, 1.0f, 4.0f}, EDITOR);
	sceneCam->init(window, glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 30.0f));

	// Initialize the skybox.
	std::vector<std::string> cubeTex
  {
    "./res/textures/skybox/right.jpg",
    "./res/textures/skybox/left.jpg",
    "./res/textures/skybox/top.jpg",
    "./res/textures/skybox/bottom.jpg",
    "./res/textures/skybox/front.jpg",
    "./res/textures/skybox/back.jpg"
  };
	skybox = new EnvironmentMap("./res/r_shaders/skybox.vs",
													    "./res/r_shaders/skybox.fs",
															"./res/models/cube.obj");
	skybox->loadCubeMap(cubeTex, SKYBOX);
	skybox->precomputeIrradiance();

	// Load the obj file(s).
	ground.loadOBJFile("./res/models/ground_plane.obj", false);
	objModel1.loadOBJFile("./res/models/bunny.obj", false);
	objModel1.normalizeVertices();
	objModel1.moveMesh(glm::vec3(-2.0f, 0.0f, 0.0f));
	objModel2.loadOBJFile("./res/models/teapot.obj", false);
	objModel2.normalizeVertices();

	// The textures for the sphere.
	textures = new Texture2DController();

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

	// Draw the ground plane.
	renderer->draw(&ground, program, sceneCam);

	// Draw the bunny.
	renderer->draw(&objModel1, program, sceneCam);

	// Draw the teapot.
	renderer->draw(&objModel2, program, sceneCam);

	// Draw the skybox.
	skybox->draw(sceneCam);

	// Second pass for post-processing.
  renderer->drawToViewPort(drawBuffer);
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

	drawBuffer->resize(w, h);
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

int main(int argc, char **argv)
{
	// Set the error callback so we get output when things go wrong.
	glfwSetErrorCallback(error_callback);

	// Initialize GLFW.
	if (!glfwInit())
		fprintf(stderr, "Can't initialize GLFW\n");

	// Open the window using GLFW.
	window = glfwCreateWindow(width, height, "Viewport", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Initialize GLEW.
	glfwMakeContextCurrent(window);
	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		printf("Error starting GLEW: %s\n",glewGetErrorString(error));
		exit(0);
	}

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, width, height);

	init();
	gui = new GuiHandler(lights);
	gui->init(window);

	glfwSwapInterval(1);

	// Set the remaining callbacks.
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Main application loop.
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		sceneCam->mouseAction(window);
		sceneCam->keyboardAction(window);

		display();
		gui->drawGUI();

		renderer->swap(window);
	}

	gui->shutDown();

	// Cleanup pointers.
	delete program;
	delete PBR;
	delete vpPassthrough;
	delete lights;
	delete textures;
	delete gui;
	delete drawBuffer;
	delete renderer;
	delete skybox;

	glfwTerminate();
}
