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
    void geometryPass();
    void shadowPass();
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

      // The full-screen quad for rendering.
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

      // Prepare the shadow buffers.
      auto dSpec = FBOCommands::getDefaultDepthSpec();
      for (unsigned int i = 0; i < NUM_CASCADES; i++)
      {
        storage->shadowBuffer[i] = FrameBuffer(2048, 2048);
        storage->shadowBuffer[i].attachTexture2D(dSpec);
        storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Depth);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
      }
      storage->hasCascades = false;

      storage->width = width;
      storage->height = height;

      // The geoemtry pass framebuffer.
      storage->geometryPass = FrameBuffer(width, height);
      // The position texture.
      auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
      storage->geometryPass.attachTexture2D(cSpec);
      // The normal texture.
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1);
      storage->geometryPass.attachTexture2D(cSpec);
      // The albedo texture.
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour2);
      storage->geometryPass.attachTexture2D(cSpec);
      // The lighting materials texture.
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour3);
      // The ID texture with a mask for the current selected entity.
      storage->geometryPass.attachTexture2D(cSpec);
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour4);
      cSpec.sWrap = TextureWrapParams::ClampEdges;
      cSpec.tWrap = TextureWrapParams::ClampEdges;
      storage->geometryPass.attachTexture2D(cSpec);
      storage->geometryPass.setDrawBuffers();
    	storage->geometryPass.attachTexture2D(dSpec);

      // The lighting pass framebuffer.
      storage->lightingPass = FrameBuffer(width, height);
      cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
      storage->lightingPass.attachTexture2D(cSpec);
      storage->lightingPass.attachRenderBuffer();

      // Shaders for the various passes.
      storage->shadowShader = shaderCache->getAsset("shadow_shader");

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
      storage->directionalShader->addUniformSampler("cascadeMaps[0]", 7);
      storage->directionalShader->addUniformSampler("cascadeMaps[1]", 8);
      storage->directionalShader->addUniformSampler("cascadeMaps[2]", 9);

      storage->hdrPostShader = shaderCache->getAsset("post_hdr");
      storage->hdrPostShader->addUniformSampler("screenColour", 0);
      storage->hdrPostShader->addUniformSampler("entityIDs", 1);

      storage->outlineShader = shaderCache->getAsset("post_entity_outline");
      storage->outlineShader->addUniformSampler("entity", 0);
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
      storage->drawEdge = false;

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
        storage->renderQueue.clear();
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
        geometryPass();

        shadowPass();

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

      RendererCommands::drawPrimatives(PrimativeType::Triangle, data->numToRender());

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

    void
    submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                GLfloat id, bool drawSelectionMask)
    {
      storage->renderQueue.emplace_back(data, &materials, model, id, drawSelectionMask);
      storage->shadowQueue.emplace_back(data, model);
    }

    void
    submit(DirectionalLight light)
    {
      DirectionalLight temp = light;
      temp.direction = -1.0f * light.direction;

      storage->directionalQueue.push_back(temp);
      stats->numDirLights++;
    }

    void
    submit(PointLight light, const glm::mat4 &model)
    {
      PointLight temp = light;
      temp.position = glm::vec3(model * glm::vec4(light.position, 1.0f));

      storage->pointQueue.push_back(temp);
      stats->numPointLights++;
    }

    void
    submit(SpotLight light, const glm::mat4 &model)
    {
      SpotLight temp = light;
      temp.direction = -1.0f * light.direction;
      temp.position = glm::vec3(model * glm::vec4(light.position, 1.0f));

      storage->spotQueue.push_back(temp);
      stats->numSpotLights++;
    }

    //--------------------------------------------------------------------------
    // Deferred geometry pass.
    //--------------------------------------------------------------------------
    void geometryPass()
    {
      storage->geometryPass.clear();
      storage->geometryPass.bind();
      storage->geometryPass.setViewport();

      for (auto& drawable : storage->renderQueue)
      {
        auto& [data, materials, transform, id, drawSelectionMask] = drawable;
        for (auto& pair : data->getSubmeshes())
        {
          Material* material = materials->getMaterial(pair.first);
          if (!material)
          {
            // Generate a material for the submesh if it doesn't have one.
            materials->attachMesh(pair.first);
            material = materials->getMaterial(pair.first);
          }

          Shader* program = storage->geometryShader;

          material->getVec3("camera.position") = storage->sceneCam->getCamPos();
          material->getMat3("normalMat") = glm::transpose(glm::inverse(glm::mat3(transform)));
          material->getMat4("model") = transform;
          material->getMat4("mVP") = storage->sceneCam->getProjMatrix() * storage->sceneCam->getViewMatrix() * transform;
          material->getFloat("uID") = id + 1.0f;
          if (drawSelectionMask)
          {
            // Enable edge detection for selected mesh outlines.
            storage->drawEdge = true;
            material->getVec3("uMaskColour") = glm::vec3(1.0f);
          }
          else
            material->getVec3("uMaskColour") = glm::vec3(0.0f);

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

      storage->geometryPass.unbind();
    }

    //--------------------------------------------------------------------------
    // Deferred shadow mapping pass. Cascaded shadows for a "primary light".
    // TODO: Compute the scene AABB and factor that in for cascade ortho
    // calculations.
    //--------------------------------------------------------------------------
    void
    shadowPass()
    {
      //------------------------------------------------------------------------
      // Directional light shadow cascade calculations:
      //------------------------------------------------------------------------
      // https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows
      // /chapter-10-parallel-split-shadow-maps-programmable-gpus
      //------------------------------------------------------------------------
      // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/
      // cascaded-shadow-maps
      //------------------------------------------------------------------------
      const float near = storage->sceneCam->getNear();
      const float far = storage->sceneCam->getFar();

      glm::mat4 camView = storage->sceneCam->getViewMatrix();
      glm::mat4 camProj = storage->sceneCam->getProjMatrix();
      glm::mat4 camInvVP = glm::inverse(camProj * camView);

      float cascadeSplits[NUM_CASCADES];
      for (unsigned int i = 0; i < NUM_CASCADES; i++)
      {
        float p = (i + 1.0f) / (float) NUM_CASCADES;
        float log = near * std::pow(far / near, p);
        float uniform = near + (far - near) * p;
        float d = 0.91f * (log - uniform) + uniform;
        cascadeSplits[i] = (d - near) / (far - near);
      }

      // Compute the scene AABB in world space. This fixes issues with objects
      // not being captured if they're out of the camera frustum (since they
      // still need to cast shadows).
      glm::vec3 minPos = glm::vec3(std::numeric_limits<float>::max());
      glm::vec3 maxPos = glm::vec3(std::numeric_limits<float>::min());
      for (auto& pair : storage->shadowQueue)
      {
        glm::mat4 mMatrix = pair.second;
        minPos = glm::min(minPos, glm::vec3(mMatrix * glm::vec4(pair.first->getMinPos(), 1.0f)));
        maxPos = glm::max(maxPos, glm::vec3(mMatrix * glm::vec4(pair.first->getMaxPos(), 1.0f)));
      }

      float sceneMaxRadius = glm::length(minPos);
      sceneMaxRadius = glm::max(sceneMaxRadius, glm::length(maxPos));

      // Compute the lightspace matrices for each light for each cascade.
      storage->hasCascades = false;
      for (auto& dirLight : storage->directionalQueue)
      {
        float previousCascadeDistance = 0.0f;

        if (!dirLight.castShadows || !dirLight.primaryLight)
          continue;

        storage->hasCascades = true;

        glm::mat4 cascadeViewMatrix[NUM_CASCADES];
        glm::mat4 cascadeProjMatrix[NUM_CASCADES];

        for (unsigned int i = 0; i < NUM_CASCADES; i++)
        {
          glm::vec4 frustumCorners[8] =
          {
            // The near face of the camera frustum in NDC.
            { 1.0f, 1.0f, -1.0f, 1.0f },
            { -1.0f, 1.0f, -1.0f, 1.0f },
            { 1.0f, -1.0f, -1.0f, 1.0f },
            { -1.0f, -1.0f, -1.0f, 1.0f },

            // The far face of the camera frustum in NDC.
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { -1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, -1.0f, 1.0f, 1.0f },
            { -1.0f, -1.0f, 1.0f, 1.0f }
          };

          // Compute the worldspace frustum corners.
          for (unsigned int j = 0; j < 8; j++)
          {
            glm::vec4 worldDepthless = camInvVP * frustumCorners[j];
            frustumCorners[j] = worldDepthless / worldDepthless.w;
          }

          // Scale the frustum to the size of the cascade.
          for (unsigned int j = 0; j < 4; j++)
          {
            glm::vec4 distance = frustumCorners[j + 4] - frustumCorners[j];
            frustumCorners[j + 4] = frustumCorners[j] + (distance * cascadeSplits[i]);
            frustumCorners[j] = frustumCorners[j] + (distance * previousCascadeDistance);
          }

          // Find the center of the cascade frustum.
          glm::vec4 cascadeCenter = glm::vec4(0.0f);
          for (unsigned int j = 0; j < 8; j++)
            cascadeCenter += frustumCorners[j];
          cascadeCenter /= 8.0f;

          // Find the minimum and maximum size of the cascade ortho matrix.
          float radius = 0.0f;
          for (unsigned int j = 0; j < 8; j++)
          {
            float distance = glm::length(glm::vec3(frustumCorners[j] - cascadeCenter));
            radius = glm::max(radius, distance);
          }
          radius = std::ceil(radius);
          glm::vec3 maxDims = glm::vec3(radius);
          glm::vec3 minDims = -1.0f * maxDims;

          if (radius > sceneMaxRadius)
          {
            cascadeViewMatrix[i] = glm::lookAt(glm::vec3(cascadeCenter) - glm::normalize(dirLight.direction) * minDims.z,
                                               glm::vec3(cascadeCenter), glm::vec3(0.0f, 0.0f, 1.0f));
            cascadeProjMatrix[i] = glm::ortho(minDims.x, maxDims.x, minDims.y,
                                              maxDims.y, -15.0f, maxDims.z - minDims.z + 15.0f);
          }
          else
          {
            cascadeViewMatrix[i] = glm::lookAt(glm::vec3(cascadeCenter) + glm::normalize(dirLight.direction) * sceneMaxRadius,
                                               glm::vec3(cascadeCenter), glm::vec3(0.0f, 0.0f, 1.0f));
            cascadeProjMatrix[i] = glm::ortho(minDims.x, maxDims.x, minDims.y,
                                              maxDims.y, -15.0f, 2.0f * sceneMaxRadius + 15.0f);
          }

          // Offset the matrix to texel space to fix shimmering:
          //--------------------------------------------------------------------
          // https://stackoverflow.com/questions/33499053/cascaded-shadow-map-
          // shimmering
          //--------------------------------------------------------------------
          // https://docs.microsoft.com/en-ca/windows/win32/dxtecharts/common-
          // techniques-to-improve-shadow-depth-maps?redirectedfrom=MSDN
          //--------------------------------------------------------------------
          glm::mat4 lightVP = cascadeProjMatrix[i] * cascadeViewMatrix[i];
          glm::vec4 shadowOrigin = glm::vec4(glm::vec3(0.0f), 1.0f);
          shadowOrigin = lightVP * shadowOrigin;
          GLfloat storedShadowW = shadowOrigin.w;
          shadowOrigin = 0.5f * shadowOrigin * storage->shadowBuffer[i].getSize().x;

          glm::vec4 roundedShadowOrigin = glm::round(shadowOrigin);
          glm::vec4 roundedShadowOffset = roundedShadowOrigin - shadowOrigin;
          roundedShadowOffset = 2.0f * roundedShadowOffset / storage->shadowBuffer[i].getSize().x;
          roundedShadowOffset.z = 0.0f;
          roundedShadowOffset.w = 0.0f;

          glm::mat4 texelSpaceOrtho = cascadeProjMatrix[i];
          texelSpaceOrtho[3] += roundedShadowOffset;
          cascadeProjMatrix[i] = texelSpaceOrtho;

          storage->cascades[i] = cascadeProjMatrix[i] * cascadeViewMatrix[i];

          previousCascadeDistance = cascadeSplits[i];

          storage->cascadeSplits[i] = near + (cascadeSplits[i] * (far - near));
        }
      }

      // Actual shadow pass.
      for (unsigned int i = 0; i < NUM_CASCADES; i++)
      {
        storage->shadowBuffer[i].clear();
        storage->shadowBuffer[i].bind();
        storage->shadowBuffer[i].setViewport();

        if (storage->hasCascades)
        {
          for (auto& pair : storage->shadowQueue)
          {
            storage->shadowShader->addUniformMatrix("lightVP", storage->cascades[i], GL_FALSE);
            storage->shadowShader->addUniformMatrix("model", pair.second, GL_FALSE);

            for (auto& submesh : pair.first->getSubmeshes())
            {
              if (submesh.second->hasVAO())
                Renderer3D::draw(submesh.second->getVAO(), storage->shadowShader);
              else
              {
                submesh.second->generateVAO();
                if (submesh.second->hasVAO())
                  Renderer3D::draw(submesh.second->getVAO(), storage->shadowShader);
              }
            }
          }
        }

        storage->shadowBuffer[i].unbind();
      }

      storage->shadowQueue.clear();
    }

    //--------------------------------------------------------------------------
    // Deferred lighting pass.
    //--------------------------------------------------------------------------
    void
    lightingPass()
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
      RendererCommands::enable(RendererFunction::Blending);
      RendererCommands::blendEquation(BlendEquation::Additive);
      RendererCommands::blendFunction(BlendFunction::One, BlendFunction::One);

      // Screen size.
      storage->directionalShader->addUniformVector("screenSize", storage->lightingPass.getSize());
      // Camera position.
      storage->directionalShader->addUniformVector("camera.position", storage->sceneCam->getCamPos());
      // Camera view-projection matrix.
      storage->directionalShader->addUniformMatrix("camera.cameraView", storage->sceneCam->getViewMatrix(), GL_FALSE);
      // Setting the uniforms for cascaded shadows.
      for (unsigned int i = 0; i < NUM_CASCADES; i++)
      {
        if (storage->hasCascades)
        {
          storage->directionalShader->addUniformMatrix(
            (std::string("lightVP[") + std::to_string(i) + std::string("]")).c_str(),
            storage->cascades[i], GL_FALSE);

          storage->directionalShader->addUniformFloat(
            (std::string("cascadeSplits[") + std::to_string(i) + std::string("]")).c_str(),
            storage->cascadeSplits[i]);

          storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Depth, i + 7);
        }
        else
        {
          storage->directionalShader->addUniformMatrix(
            (std::string("lightVP[") + std::to_string(i) + std::string("]")).c_str(),
            glm::mat4(1.0f), GL_FALSE);
          storage->directionalShader->addUniformFloat(
            (std::string("cascadeSplits[") + std::to_string(i) + std::string("]")).c_str(),
            0.0f);
        }
      }

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
      RendererCommands::disable(RendererFunction::Blending);

      //------------------------------------------------------------------------
      // Draw the skybox.
      //------------------------------------------------------------------------
      RendererCommands::enable(RendererFunction::DepthTest);
      storage->geometryPass.blitzToOther(storage->lightingPass, FBOTargetParam::Depth);
      drawEnvironment();

      storage->lightingPass.unbind();
    }

    //--------------------------------------------------------------------------
    // Post processing pass.
    //--------------------------------------------------------------------------
    void
    postProcessPass(Shared<FrameBuffer> frontBuffer)
    {
      frontBuffer->clear();
      frontBuffer->bind();
      frontBuffer->setViewport();

      //------------------------------------------------------------------------
      // HDR post processing pass. Also streams the entity IDs to the editor
      // buffer.
      //------------------------------------------------------------------------
      storage->hdrPostShader->addUniformVector("screenSize", frontBuffer->getSize());
      storage->lightingPass.bindTextureID(FBOTargetParam::Colour0, 0);
      storage->geometryPass.bindTextureID(FBOTargetParam::Colour4, 1);

      draw(&storage->fsq, storage->hdrPostShader);

      //------------------------------------------------------------------------
      // Edge detection post processing pass. Draws an outline around the
      // selected entity.
      //------------------------------------------------------------------------
      if (storage->drawEdge)
      {
        RendererCommands::enable(RendererFunction::Blending);
        RendererCommands::blendEquation(BlendEquation::Additive);
        RendererCommands::blendFunction(BlendFunction::One, BlendFunction::One);
        RendererCommands::disable(RendererFunction::DepthTest);

        storage->outlineShader->addUniformVector("screenSize", frontBuffer->getSize());
        storage->geometryPass.bindTextureID(FBOTargetParam::Colour4, 0);

        draw(&storage->fsq, storage->outlineShader);

        RendererCommands::enable(RendererFunction::DepthTest);
        RendererCommands::disable(RendererFunction::Blending);
      }

      frontBuffer->unbind();
    }
  }
}
