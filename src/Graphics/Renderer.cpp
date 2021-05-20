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
  { }

  // Initialize the renderer.
  void
  Renderer3D::init(const GLuint width, const GLuint height)
  {
    // Initialize OpenGL parameters.
    RendererCommands::enable(RendererFunction::DepthTest);
    RendererCommands::enable(RendererFunction::CubeMapSeamless);

    // Initialize the vewport shader passthrough.
    this->viewportProgram. = createShared<Shader>("./res/shaders/viewport.vs",
                                                  "./res/shaders/viewport.fs");
  }

  // Draw the data to the screen.
  void
  Renderer3D::draw(Shared<VertexArray> data, Shared<Shader> program)
  {
    data->bind();
    program->bind();

    glDrawElements(GL_TRIANGLES, data->numToRender(), GL_UNSIGNED_INT, nullptr);

    data->unbind();
    program->unbind();
  }

  void
  Renderer3D::draw(Shared<Mesh> data, Shared<Shader> program, Shared<Camera> camera)
  {
    program->bind();
    glm::mat4 model = data->getModelMatrix();
  	glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
  	glm::mat4 modelViewPerspective = camera->getProjMatrix() * camera->getViewMatrix() * model;

  	program->addUniformMatrix("model", model, GL_FALSE);
  	program->addUniformMatrix("mVP", modelViewPerspective, GL_FALSE);
  	program->addUniformMatrix("normalMat", normal, GL_FALSE);
    program->addUniformVector("camera.position", camera->getCamPos());

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
  Renderer3D::draw(Shared<EnvironmentMap> environment, Shared<Camera> camera)
  {
    RendererCommands::depthFunction(DepthFunctions::LEq);
    environment->configure(camera);
    this->draw(environment->getCubeMesh()->getVAO(), environment->getCubeProg());
    RendererCommands::depthFunction(DepthFunctions::Less);
  }

  void
  RendererCommands::enable(const RendererFunction &toEnable)
  {
    glEnable(static_cast<GLenum>(toEnable));
  }

  void
  RendererCommands::depthFunction(const DepthFunctions &function)
  {
    glDepthFunc(static_cast<GLenum>(function));
  }

  void
  RendererCommands::setClearColour(const glm::vec4 &colour)
  {
    glClearColor(colour.r, colour.b, colour.g, colour.a);
  }

  void
  RendererCommands::clear(const bool &clearColour, const bool &clearDepth,
                          const bool &clearStencil)
  {
    if (clearColour)
      glClear(GL_COLOR_BUFFER_BIT);
    if (clearDepth)
      glClear(GL_DEPTH_BUFFER_BIT);
    if (clearStencil)
      glClear(GL_STENCIL_BUFFER_BIT);
  }

  void
  RendererCommands::setViewport(const glm::ivec2 topRight,
                                const glm::ivec2 bottomLeft)
  {
    glViewport(bottomLeft.x, bottomLeft.y, topRight.x, topRight.y);
  }
}
