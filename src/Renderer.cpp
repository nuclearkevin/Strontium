#include "Renderer.h"

using namespace SciRenderer;

Renderer* Renderer::instance = nullptr;

// Get the renderer instance.
Renderer*
Renderer::getInstance()
{
  if (instance == nullptr)
  {
    instance = new Renderer();
    return instance;
  }
  else
    return instance;
}

Renderer::~Renderer()
{
  glDeleteBuffers(1, &this->viewportVBOID);
  glDeleteVertexArrays(1, &this->viewportVAOID);
  delete this->viewportProgram;
}

// Initialize the renderer.
void
Renderer::init(const char* vertPath, const char* fragPath)
{
  // Initialize OpenGL parameters.
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  // Initialize the vewport shader passthrough.
  this->viewportProgram = new Shader(vertPath, fragPath);

  // Setup the VAO for the full screen quad.
  glGenVertexArrays(1, &this->viewportVAOID);
  glGenBuffers(1, &this->viewportVBOID);
  glBindVertexArray(this->viewportVAOID);
  glBindBuffer(GL_ARRAY_BUFFER, this->viewportVBOID);
  glBufferData(GL_ARRAY_BUFFER, sizeof(this->viewport), &this->viewport,
               GL_STATIC_DRAW);

  // Setup the shader program with the attributes.
  this->viewportProgram->addAtribute("vPosition", VEC2, GL_FALSE, 4 * sizeof(float), 0);
  this->viewportProgram->addAtribute("vTexCoords", VEC2, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));
  this->viewportProgram->addUniformSampler2D("screenTexture", 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw the data to the screen.
void
Renderer::draw(VertexArray* data, Shader* program)
{
  data->bind();
  program->bind();

  glDrawElements(GL_TRIANGLES, data->numToRender(), GL_UNSIGNED_INT, nullptr);

  data->unbind();
  program->unbind();
}

void
Renderer::draw(Mesh* data, Shader* program, Camera* camera)
{
  program->bind();
  glm::mat4 model = data->getModelMatrix();
	glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
	glm::mat4 modelViewPerspective = camera->getProjMatrix() * camera->getViewMatrix() * model;

	program->addUniformMatrix("model", model, GL_FALSE);
	program->addUniformMatrix("mVP", modelViewPerspective, GL_FALSE);
	program->addUniformMatrix("normalMat", normal, GL_FALSE);

  if (data->hasVAO())
    this->draw(data->getVAO(), program);
  else
  {
    data->generateVAO(program);
    if (data->hasVAO())
      this->draw(data->getVAO(), program);
  }
}

void
Renderer::drawToViewPort(FrameBuffer* drawBuffer)
{
  drawBuffer->unbind();
  glDisable(GL_DEPTH_TEST);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

	viewportProgram->bind();
  glBindVertexArray(this->viewportVAOID);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, drawBuffer->getColourID());
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_DEPTH_TEST);
}

void
Renderer::drawToViewPort(GLuint texID)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_DEPTH_TEST);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

	viewportProgram->bind();
  glBindVertexArray(this->viewportVAOID);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texID);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_DEPTH_TEST);
}

void
Renderer::swap(GLFWwindow* window)
{
  glfwSwapBuffers(window);
}
