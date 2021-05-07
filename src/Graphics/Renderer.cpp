#include "Graphics/Renderer.h"

namespace SciRenderer
{
  Renderer3D* Renderer3D::instance = nullptr;

  // Get the renderer instance.
  Renderer3D*
  Renderer3D::getInstance()
  {
    if (instance == nullptr)
    {
      instance = new Renderer3D();
      return instance;
    }
    else
      return instance;
  }

  Renderer3D::~Renderer3D()
  {
    glDeleteBuffers(1, &this->viewportVBOID);
    glDeleteVertexArrays(1, &this->viewportVAOID);
    delete this->viewportProgram;
  }

  // Initialize the renderer.
  void
  Renderer3D::init(const std::string &vertPath, const std::string &fragPath)
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
  Renderer3D::draw(VertexArray* data, Shader* program)
  {
    data->bind();
    program->bind();

    glDrawElements(GL_TRIANGLES, data->numToRender(), GL_UNSIGNED_INT, nullptr);

    data->unbind();
    program->unbind();
  }

  void
  Renderer3D::draw(Mesh* data, Shader* program, Camera* camera)
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

  // Draw the contents of a framebuffer to the default buffer.
  void
  Renderer3D::drawToViewPort(FrameBuffer* drawBuffer)
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
    glBindVertexArray(0);
  	glEnable(GL_DEPTH_TEST);
  }

  // Draws a full screen quad with a given program using whatever buffer is
  // currently bound.
  void
  Renderer3D::drawFSQ(Shader* program)
  {
    program->bind();
    glBindVertexArray(this->viewportVAOID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
  }

  // Draw a texture to the default buffer.
  void
  Renderer3D::debugDrawTex(GLuint texID)
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

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
  	glEnable(GL_DEPTH_TEST);
  }

  void
  Renderer3D::swap(GLFWwindow* window)
  {
    glfwSwapBuffers(window);
  }
}
