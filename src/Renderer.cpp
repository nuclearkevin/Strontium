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

// Initialize the renderer.
void
Renderer::init(GLenum mode)
{
  glEnable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, mode);
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
Renderer::swap(GLFWwindow* window)
{
  glfwSwapBuffers(window);
}

void
Renderer::clear()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
