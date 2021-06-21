#include "Graphics/Renderer.h"

// Project includes.
#include "Core/AssetManager.h"

namespace SciRenderer
{
  namespace Renderer3D
  {
    RendererStorage* storage;
    RendererState* state;
    RendererStats* stats;

    // Initialize the renderer.
    void
    init(const GLuint width, const GLuint height)
    {
      // Initialize OpenGL parameters.
      RendererCommands::enable(RendererFunction::DepthTest);
      RendererCommands::enable(RendererFunction::CubeMapSeamless);

      storage = new RendererStorage();
      state = new RendererState();
      stats = new RendererStats();

      // Initialize the vewport shader passthrough.
      auto shaderCache = AssetManager<Shader>::getManager();

      shaderCache->attachAsset("fsq_shader", new Shader("./assets/shaders/viewport.vs",
                                                        "./assets/shaders/viewport.fs"));
    }

    // Shutdown the renderer.
    void
    shutdown()
    {
      delete storage;
      delete state;
      delete stats;
    }

    // Get the storage.
    RendererStorage* getStorage()
    {
      return storage;
    }

    // Get the renderer state, settings and statistics.
    RendererState* getState()
    {
      return state;
    }

    RendererStats* getStats()
    {
      return stats;
    }

    // Generic begin and end for the renderer.
    void
    begin()
    {
      // Reset the stats each frame.
      stats->drawCalls = 0;
      stats->numVertices = 0;
      stats->numTriangles = 0;
    }

    void
    end()
    {

    }

    // Draw the data to the screen.
    void
    draw(VertexArray* data, Shader* program)
    {
      data->bind();
      program->bind();

      glDrawElements(GL_TRIANGLES, data->numToRender(), GL_UNSIGNED_INT, nullptr);

      data->unbind();
      program->unbind();
    }

    // Draw a model to the screen (it just draws all the submeshes associated with the model).
    void
    draw(Model* data, ModelMaterial &materials, const glm::mat4 &model,
         Shared<Camera> camera)
    {
      for (auto& pair : data->getSubmeshes())
      {
        Material* material = materials.getMaterial(pair.second);
        Shader* program = material->getShader();

        material->configure();

        // Pass the camera uniforms into the material's shader program.
        glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
      	glm::mat4 modelViewPerspective = camera->getProjMatrix() * camera->getViewMatrix() * model;

      	program->addUniformMatrix("model", model, GL_FALSE);
      	program->addUniformMatrix("mVP", modelViewPerspective, GL_FALSE);
      	program->addUniformMatrix("normalMat", normal, GL_FALSE);
        program->addUniformVector("camera.position", camera->getCamPos());

        if (pair.second->hasVAO())
          Renderer3D::draw(pair.second->getVAO(), program);
        else
        {
          pair.second->generateVAO();
          if (pair.second->hasVAO())
            Renderer3D::draw(pair.second->getVAO(), program);
        }

        stats->drawCalls++;
        stats->numVertices += pair.second->getData().size();
        stats->numTriangles += pair.second->getIndices().size() / 3;
      }
    }

    // Draw an environment map to the screen. Draws all the submeshes associated
    // with the cube model.
    void
    drawEnvironment(Shared<Camera> camera)
    {
      RendererCommands::depthFunction(DepthFunctions::LEq);
      storage->currentEnvironment->configure(camera);

      for (auto& pair : storage->currentEnvironment->getCubeMesh()->getSubmeshes())
        Renderer3D::draw(pair.second->getVAO(), storage->currentEnvironment->getCubeProg());

      RendererCommands::depthFunction(DepthFunctions::Less);
    }
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
