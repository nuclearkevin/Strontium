#include "Graphics/Renderer.h"

// Project includes.
#include "Core/AssetManager.h"

namespace SciRenderer
{
  //----------------------------------------------------------------------------
  // 3D renderer starts here.
  //----------------------------------------------------------------------------
  namespace Renderer3D
  {
    // Forward declaration for passes.
    void lightingPass();
    void postProcessPass(Shared<FrameBuffer> frontBuffer);

    // Draw the data given, forward rendering style.
    void draw(VertexArray* data, Shader* program);
    void drawEnvironment();

    RendererStorage* storage;
    RendererState* state;
    RendererStats* stats;

    // Initialize the renderer.
    void
    init(const GLuint width, const GLuint height)
    {
      // Initialize the vewport shader passthrough.
      auto shaderCache = AssetManager<Shader>::getManager();

      // Initialize OpenGL parameters.
      RendererCommands::enable(RendererFunction::DepthTest);
      RendererCommands::enable(RendererFunction::CubeMapSeamless);

      // Setup the various storage structs.
      storage = new RendererStorage();
      state = new RendererState();
      stats = new RendererStats();

      GLfloat fsqVertices[] =
      {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f, 1.0f
      };

      GLuint fsqIndices[] =
      {
        0, 1, 2, 2, 3, 0
      };

      storage->fsq = VertexArray(fsqVertices, 8 * sizeof(GLfloat), BufferType::Dynamic);
      storage->fsq.addIndexBuffer(fsqIndices, 6, BufferType::Dynamic);
      storage->fsq.addAttribute(0, AttribType::Vec2, GL_FALSE, 2 * sizeof(GLfloat), 0);

      // Prepare the deferred renderer.
      storage->width = width;
      storage->height = height;
      storage->geometryPass = FrameBuffer(width, height);

      auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
      storage->geometryPass.attachTexture2D(cSpec);
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1);
      storage->geometryPass.attachTexture2D(cSpec);
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour2);
      storage->geometryPass.attachTexture2D(cSpec);
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour3);
      storage->geometryPass.attachTexture2D(cSpec);
    	storage->geometryPass.attachRenderBuffer();
      storage->geometryPass.setDrawBuffers();

      storage->geometryShader = shaderCache->getAsset("geometry_pass_shader");

      storage->ambientShader = shaderCache->getAsset("deferred_ambient");
      storage->ambientShader->addUniformSampler("irradianceMap", 0);
      storage->ambientShader->addUniformSampler("reflectanceMap", 1);
      storage->ambientShader->addUniformSampler("brdfLookUp", 2);
      storage->ambientShader->addUniformSampler("gPosition", 3);
      storage->ambientShader->addUniformSampler("gNormal", 4);
      storage->ambientShader->addUniformSampler("gAlbedo", 5);
      storage->ambientShader->addUniformSampler("gMatProp", 6);

      storage->directionalShader = shaderCache->getAsset("deferred_directional");
      storage->directionalShader->addUniformSampler("gPosition", 3);
      storage->directionalShader->addUniformSampler("gNormal", 4);
      storage->directionalShader->addUniformSampler("gAlbedo", 5);
      storage->directionalShader->addUniformSampler("gMatProp", 6);

      storage->hdrPostShader = shaderCache->getAsset("post_hdr");
      storage->hdrPostShader->addUniformSampler("screenColour", 0);

      storage->lightingPass = FrameBuffer(width, height);
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
      storage->lightingPass.attachTexture2D(cSpec);
      storage->lightingPass.attachRenderBuffer();
    }

    // Shutdown the renderer.
    void
    shutdown()
    {
      delete storage;
      delete state;
      delete stats;
    }

    // Get the renderer storage, state and stats.
    RendererStorage* getStorage() { return storage; }
    RendererState* getState() { return state; }
    RendererStats* getStats() { return stats; }

    // Generic begin and end for the renderer.
    void
    begin(GLuint width, GLuint height, Shared<Camera> sceneCam, bool isForward)
    {
      storage->sceneCam = sceneCam;

      // Resize the framebuffer at the start of a frame, if required.
      storage->width = width;
      storage->height = height;
      storage->isForward = isForward;

      auto geoSize = storage->geometryPass.getSize();
      if (geoSize.x != width || geoSize.y != height)
      {
        storage->geometryPass.resize(width, height);
        storage->lightingPass.resize(width, height);
      }

      // Reset the stats each frame.
      stats->drawCalls = 0;
      stats->numVertices = 0;
      stats->numTriangles = 0;
      stats->numDirLights = 0;
      stats->numPointLights = 0;
      stats->numSpotLights = 0;

      if (isForward)
      {
        storage->lightingPass.clear();
        storage->lightingPass.bind();
        storage->lightingPass.setViewport();

        storage->currentEnvironment->bind(MapType::Irradiance, 0);
        storage->currentEnvironment->bind(MapType::Prefilter, 1);
        storage->currentEnvironment->bind(MapType::Integration, 2);
      }
      else
      {
        storage->geometryPass.clear();
        storage->geometryPass.bind();
        storage->geometryPass.setViewport();
      }
    }

    void
    end(Shared<FrameBuffer> frontBuffer)
    {
      if (storage->isForward)
      {
        drawEnvironment();
        storage->lightingPass.unbind();
      }
      else
      {
        storage->geometryPass.unbind();

        lightingPass();

        postProcessPass(frontBuffer);
      }
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

    // Draw an environment map to the screen. Draws all the submeshes associated
    // with the cube model.
    void
    drawEnvironment()
    {
      RendererCommands::depthFunction(DepthFunctions::LEq);
      storage->currentEnvironment->configure(storage->sceneCam);

      for (auto& pair : storage->currentEnvironment->getCubeMesh()->getSubmeshes())
        Renderer3D::draw(pair.second->getVAO(), storage->currentEnvironment->getCubeProg());

      RendererCommands::depthFunction(DepthFunctions::Less);
    }

    void submit(Model* data, ModelMaterial &materials, const glm::mat4 &model)
    {
      for (auto& pair : data->getSubmeshes())
      {
        Material* material = materials.getMaterial(pair.first);
        if (!material)
        {
          // Generate a material for the submesh if it doesn't have one.
          materials.attachMesh(pair.first);
          material = materials.getMaterial(pair.first);
        }

        Shader* program;
        if (storage->isForward)
          program = material->getShader();
        else
          program = storage->geometryShader;

        material->getVec3("camera.position") = storage->sceneCam->getCamPos();
        material->getMat3("normalMat") = glm::transpose(glm::inverse(glm::mat3(model)));
        material->getMat4("model") = model;
        material->getMat4("mVP") = storage->sceneCam->getProjMatrix() * storage->sceneCam->getViewMatrix() * model;
        material->configure(program);

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

    void submit(DirectionalLight light)
    {
      DirectionalLight temp = light;
      temp.direction = -1.0f * light.direction;

      storage->directionalQueue.push_back(temp);
      stats->numDirLights++;
    }

    void submit(PointLight light, const glm::mat4 &model)
    {
      PointLight temp = light;
      temp.position = glm::vec3(model * glm::vec4(light.position, 1.0f));

      storage->pointQueue.push_back(temp);
      stats->numPointLights++;
    }

    void submit(SpotLight light, const glm::mat4 &model)
    {
      SpotLight temp = light;
      temp.direction = -1.0f * light.direction;
      temp.position = glm::vec3(model * glm::vec4(light.position, 1.0f));

      storage->spotQueue.push_back(temp);
      stats->numSpotLights++;
    }

    void lightingPass()
    {
      RendererCommands::disable(RendererFunction::DepthTest);
      storage->lightingPass.clear();
      storage->lightingPass.bind();
      storage->lightingPass.setViewport();

      //------------------------------------------------------------------------
      // Ambient lighting subpass.
      //------------------------------------------------------------------------
      // Environment maps.
      storage->currentEnvironment->bind(MapType::Irradiance, 0);
      storage->currentEnvironment->bind(MapType::Prefilter, 1);
      storage->currentEnvironment->bind(MapType::Integration, 2);
      // Gbuffer textures.
      storage->geometryPass.bindTextureID(FBOTargetParam::Colour0, 3);
      storage->geometryPass.bindTextureID(FBOTargetParam::Colour1, 4);
      storage->geometryPass.bindTextureID(FBOTargetParam::Colour2, 5);
      storage->geometryPass.bindTextureID(FBOTargetParam::Colour3, 6);
      // Screen size.
      storage->ambientShader->addUniformVector("screenSize", storage->lightingPass.getSize());
      // Camera position.
      storage->ambientShader->addUniformVector("camera.position", storage->sceneCam->getCamPos());

      draw(&storage->fsq, storage->ambientShader);

      //------------------------------------------------------------------------
      // Directional lighting subpass.
      //------------------------------------------------------------------------
      glEnable(GL_BLEND);
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc(GL_ONE, GL_ONE);

      // Screen size.
      storage->directionalShader->addUniformVector("screenSize", storage->lightingPass.getSize());
      // Camera position.
      storage->directionalShader->addUniformVector("camera.position", storage->sceneCam->getCamPos());

      for (auto& lights : storage->directionalQueue)
      {
        storage->directionalShader->addUniformVector("lDirection", lights.direction);
        storage->directionalShader->addUniformVector("lColour", lights.colour);
        storage->directionalShader->addUniformFloat("lIntensity", lights.intensity);

        draw(&storage->fsq, storage->directionalShader);
      }

      storage->directionalQueue.clear();

      //------------------------------------------------------------------------
      // Point lighting subpass.
      //------------------------------------------------------------------------
      storage->pointQueue.clear();

      //------------------------------------------------------------------------
      // Spot lighting subpass.
      //------------------------------------------------------------------------
      storage->spotQueue.clear();
      glDisable(GL_BLEND);

      //------------------------------------------------------------------------
      // Draw the skybox.
      //------------------------------------------------------------------------
      RendererCommands::enable(RendererFunction::DepthTest);
      storage->geometryPass.blitzToOther(storage->lightingPass, FBOTargetParam::Depth);
      drawEnvironment();

      storage->lightingPass.unbind();
    }

    void postProcessPass(Shared<FrameBuffer> frontBuffer)
    {
      frontBuffer->clear();
      frontBuffer->bind();
      frontBuffer->setViewport();

      //------------------------------------------------------------------------
      // HDR post processing pass.
      //------------------------------------------------------------------------
      storage->hdrPostShader->addUniformVector("screenSize", frontBuffer->getSize());
      storage->lightingPass.bindTextureID(FBOTargetParam::Colour0, 0);

      draw(&storage->fsq, storage->hdrPostShader);

      frontBuffer->unbind();
    }
  }

  //----------------------------------------------------------------------------
  // Renderer commands start here.
  //----------------------------------------------------------------------------
  void
  RendererCommands::enable(const RendererFunction &toEnable)
  {
    glEnable(static_cast<GLenum>(toEnable));
  }

  void
  RendererCommands::disable(const RendererFunction &toDisable)
  {
    glDisable(static_cast<GLenum>(toDisable));
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
