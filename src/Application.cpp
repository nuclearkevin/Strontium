#define GLFW_DLL
#define GLFW_INCLUDE_NONE
#define GLM_FORCE_RADIANS

// STL and standard includes.
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

// OpenGL includes.
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Project includes.
#include "Shaders.h"
#include "Meshes.h"
#include "Buffers.h"
#include "Camera.h"

// Shader ID, need to abstract shaders further.
GLuint program;

// Vertex array and index buffer, need to abstract.
GLuint triangleVAO;
GLuint ibuffer;

// Window.
GLFWwindow *window;

// Projection (perspective) matrix.
glm::mat4 projection;

// Camera.
Camera* sceneCam = new Camera(512/2, 512/2, glm::vec3 {0.0f, 0.0f, 4.0f});

// Mesh data.
Mesh objModel1, objModel2;

// Buffer objects.
VertexBuffer* verBuff;
IndexBuffer*  indBuff;

void init() {
	// OpenGL program and buffer locations.
	int vs;
	int fs;

	// Load the obj file(s).
	objModel2.loadOBJFile("teapot.obj");
	objModel2.normalizeVertices();
	objModel1.loadOBJFile("bunny.obj");
	objModel1.normalizeVertices();

	// Generate the vertex and index buffers.
	verBuff = buildBatchVBuffer(std::vector<Mesh*> { &objModel1, &objModel2 }, DYNAMIC);
	indBuff = buildBatchIBuffer(std::vector<Mesh*> { &objModel1, &objModel2 }, DYNAMIC);

	// Generate the vertex array object on the GPU.
	glGenVertexArrays(1, &triangleVAO);
	glBindVertexArray(triangleVAO);
	verBuff->bind();
	indBuff->bind();

	// Link the vertex position and normal data to the variables in the NEW shader
	// programs.
	vs = buildShader(GL_VERTEX_SHADER, "test.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, "test.fs");
	program = buildProgram(vs,fs,0);
	glUseProgram(program);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
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
void display() {
	int modelLoc;
	int normalLoc;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	glm::mat4 model = glm::rotate(glm::mat4(1.0), 0.0f, glm::vec3(0.0, 1.0, 0.0));

	glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3((*sceneCam->getViewMatrix()) * model)));

	glm::mat4 modelViewPerspective = projection * (*sceneCam->getViewMatrix()) * model;

	glUseProgram(program);
	modelLoc = glGetUniformLocation(program,"model");
	glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(modelViewPerspective));
	normalLoc = glGetUniformLocation(program,"normalMat");
	glUniformMatrix3fv(normalLoc, 1, 0, glm::value_ptr(normal));

	glBindVertexArray(triangleVAO);
	indBuff->bind();
	glDrawElements(GL_TRIANGLES, indBuff->getCount(), GL_UNSIGNED_INT, nullptr);
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
	if (!glfwInit()) {
		fprintf(stderr, "can't initialize GLFW\n");
	}

	// Open the window using GLFW.
	window = glfwCreateWindow(512, 512, "OpenGL Tests!", NULL, NULL);
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

	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);
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
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete verBuff;
	delete indBuff;
	glfwTerminate();
}
