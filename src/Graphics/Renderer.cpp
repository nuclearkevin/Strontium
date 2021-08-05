#include "Graphics/Renderer.h"

// Project includes.
#include "Core/AssetManager.h"

// STL includes.
#include <chrono>

namespace Strontium
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

      storage->width = width;
      storage->height = height;

      storage->fsq = VertexArray(fsqVertices, 8 * sizeof(GLfloat), BufferType::Dynamic);
      storage->fsq.addIndexBuffer(fsqIndices, 6, BufferType::Dynamic);
      storage->fsq.addAttribute(0, AttribType::Vec2, GL_FALSE, 2 * sizeof(GLfloat), 0);

      // Prepare the shadow buffers.
      auto dSpec = FBOCommands::getDefaultDepthSpec();
      auto vSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
      vSpec.internal = TextureInternalFormats::RGBA32f;
      vSpec.format = TextureFormats::RGBA;
      for (unsigned int i = 0; i < NUM_CASCADES; i++)
      {
        storage->shadowBuffer[i] = FrameBuffer(state->cascadeSize, state->cascadeSize);
        storage->shadowBuffer[i].attachTexture2D(vSpec);
        storage->shadowBuffer[i].attachTexture2D(dSpec);
        storage->shadowBuffer[i].setClearColour(glm::vec4(1.0f));
      }
      storage->shadowEffectsBuffer = FrameBuffer(state->cascadeSize, state->cascadeSize);
      storage->shadowEffectsBuffer.attachTexture2D(vSpec);
      storage->shadowEffectsBuffer.attachTexture2D(dSpec);
      storage->shadowEffectsBuffer.setClearColour(glm::vec4(1.0f));
      storage->hasCascades = false;

      storage->gBuffer.resize(width, height);

      // The lighting pass framebuffer.
      storage->lightingPass = FrameBuffer(width, height);
      auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
      storage->lightingPass.attachTexture2D(cSpec);
      storage->lightingPass.attachRenderBuffer();

      // Prepare the various uniform buffers.
      storage->camBuffer.bindToPoint(0);
      storage->editorBuffer.bindToPoint(2);

      // Shaders for the various passes.
      storage->shadowShader = shaderCache->getAsset("shadow_shader");
      storage->geometryShader = shaderCache->getAsset("geometry_pass_shader");
      storage->ambientShader = shaderCache->getAsset("deferred_ambient");
      storage->directionalShaderShadowed = shaderCache->getAsset("deferred_directional_shadowed");
      storage->directionalShader = shaderCache->getAsset("deferred_directional");
      storage->horBlur = shaderCache->getAsset("post_hor_gaussian_blur");
      storage->verBlur = shaderCache->getAsset("post_ver_gaussian_blur");
      storage->hdrPostShader = shaderCache->getAsset("post_hdr");
      storage->outlineShader = shaderCache->getAsset("post_entity_outline");
      storage->gridShader = shaderCache->getAsset("post_grid");
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
      storage->camFrustum = buildCameraFrustum(sceneCam);

      // Resize the framebuffer at the start of a frame, if required.
      storage->isForward = isForward;
      storage->drawEdge = false;

      if (storage->width != width || storage->height != height)
      {
        storage->gBuffer.resize(width, height);
        storage->lightingPass.resize(width, height);
        storage->width = width;
        storage->height = height;
      }

      // Reset the stats each frame.
      stats->drawCalls = 0;
      stats->numVertices = 0;
      stats->numTriangles = 0;
      stats->numDirLights = 0;
      stats->numPointLights = 0;
      stats->numSpotLights = 0;
      stats->geoFrametime = 0.0f;
      stats->shadowFrametime = 0.0f;
      stats->lightFrametime = 0.0f;
      stats->postFramtime = 0.0f;

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

      for (auto& submesh : storage->currentEnvironment->getCubeMesh()->getSubmeshes())
        Renderer3D::draw(submesh.getVAO(), storage->currentEnvironment->getCubeProg());

      RendererCommands::depthFunction(DepthFunctions::Less);
    }

    void
    submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                GLfloat id, bool drawSelectionMask)
    {
      // Cull the model early.
      glm::vec3 min = glm::vec3(model * glm::vec4(data->getMinPos(), 1.0f));
      glm::vec3 max = glm::vec3(model * glm::vec4(data->getMaxPos(), 1.0f));
      glm::vec3 center = (min + max) / 2.0f;
      GLfloat radius = glm::length(min + center);

      if (boundingBoxInFrustum(storage->camFrustum, min, max) && state->frustumCull)
        storage->renderQueue.emplace_back(data, &materials, model, id, drawSelectionMask);
      else if (!state->frustumCull)
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
      auto start = std::chrono::steady_clock::now();

      // Upload camera uniforms to the camera uniform buffer.
      auto camProj = storage->sceneCam->getProjMatrix();
      auto camView = storage->sceneCam->getViewMatrix();
      auto camPos = storage->sceneCam->getCamPos();

      storage->camBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(camView));
      storage->camBuffer.setData(sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camProj));
      storage->camBuffer.setData(2 * sizeof(glm::mat4), sizeof(glm::vec3), &camPos.x);

      storage->gBuffer.beginGeoPass();

      Shader* program = storage->geometryShader;

      for (auto& drawable : storage->renderQueue)
      {
        auto& [data, materials, transform, id, drawSelectionMask] = drawable;
        for (auto& submesh : data->getSubmeshes())
        {
          // Cull the submesh if it isn't in the frustum.
          glm::vec3 min = glm::vec3(transform * glm::vec4(submesh.getMinPos(), 1.0f));
          glm::vec3 max = glm::vec3(transform * glm::vec4(submesh.getMaxPos(), 1.0f));

          if (!boundingBoxInFrustum(storage->camFrustum, min, max) && state->frustumCull)
            continue;

          Material* material = materials->getMaterial(submesh.getName());
          if (!material)
            continue;

          material->getMat4("uModel") = transform;

          glm::vec4 maskColourID;
          if (drawSelectionMask)
          {
            // Enable edge detection for selected mesh outlines.
            storage->drawEdge = true;
            maskColourID = glm::vec4(1.0f);
          }
          else
            maskColourID = glm::vec4(0.0f);

          maskColourID.w = id + 1.0f;
          storage->editorBuffer.setData(0, sizeof(glm::vec4), &maskColourID.x);

          // This is incredibly slow, ~0.02 ms per submesh.
          material->configure();

          if (submesh.hasVAO())
            Renderer3D::draw(submesh.getVAO(), program);
          else
          {
            submesh.generateVAO();
            Renderer3D::draw(submesh.getVAO(), program);
          }

          stats->drawCalls++;
          stats->numVertices += submesh.getData().size();
          stats->numTriangles += submesh.getIndices().size() / 3;
        }
      }

      storage->gBuffer.endGeoPass();

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      stats->geoFrametime = elapsed.count() * 1000.0f;
    }

    //--------------------------------------------------------------------------
    // Deferred shadow mapping pass. Cascaded shadows for a "primary light".
    //--------------------------------------------------------------------------
    void
    shadowPass()
    {
      auto start = std::chrono::steady_clock::now();
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
        float d = state->cascadeLambda * (log - uniform) + uniform;
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
      glm::vec3 lightDir = glm::vec3(0.0f);
      for (auto& dirLight : storage->directionalQueue)
      {
        if (!dirLight.castShadows || !dirLight.primaryLight)
          continue;

        storage->hasCascades = true;
        lightDir = glm::normalize(dirLight.direction);
      }

      if (storage->hasCascades)
      {
        float previousCascadeDistance = 0.0f;

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

          // Find the minimum and maximum size of the cascade ortho matrix. Bounding spheres!
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
            cascadeViewMatrix[i] = glm::lookAt(glm::vec3(cascadeCenter) - lightDir * minDims.z,
                                               glm::vec3(cascadeCenter), glm::vec3(0.0f, 0.0f, 1.0f));
            cascadeProjMatrix[i] = glm::ortho(minDims.x, maxDims.x, minDims.y,
                                              maxDims.y, -15.0f, maxDims.z - minDims.z + 15.0f);
          }
          else
          {
            cascadeViewMatrix[i] = glm::lookAt(glm::vec3(cascadeCenter) + lightDir * sceneMaxRadius,
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
              if (submesh.hasVAO())
                Renderer3D::draw(submesh.getVAO(), storage->shadowShader);
              else
              {
                submesh.generateVAO();
                if (submesh.hasVAO())
                  Renderer3D::draw(submesh.getVAO(), storage->shadowShader);
              }
            }
          }
        }
        storage->shadowBuffer[i].unbind();

        // Apply a 2-pass 9 tap Gaussian blur to the shadow map.
        // First pass (horizontal) is FBO attachment -> temp FBO.
        RendererCommands::disableDepthMask();
        RendererCommands::disable(RendererFunction::DepthTest);
        storage->shadowEffectsBuffer.clear();
        storage->shadowEffectsBuffer.bind();
        storage->shadowEffectsBuffer.setViewport();

        storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Colour0, 0);
        draw(&storage->fsq, storage->horBlur);

        storage->shadowEffectsBuffer.unbind();

        // Second pass (vertical) is temp FBO -> FBO attachment.
        storage->shadowBuffer[i].clear();
        storage->shadowBuffer[i].bind();
        storage->shadowBuffer[i].setViewport();

        storage->shadowEffectsBuffer.bindTextureID(FBOTargetParam::Colour0, 0);
        draw(&storage->fsq, storage->verBlur);

        storage->shadowBuffer[i].unbind();

        RendererCommands::enable(RendererFunction::DepthTest);
        RendererCommands::enableDepthMask();
      }
      storage->shadowQueue.clear();

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      stats->shadowFrametime = elapsed.count() * 1000.0f;
    }

    //--------------------------------------------------------------------------
    // Deferred lighting pass.
    //--------------------------------------------------------------------------
    void
    lightingPass()
    {
      auto start = std::chrono::steady_clock::now();

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
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour0, 3);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour1, 4);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour2, 5);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour3, 6);
      // Screen size.
      storage->ambientShader->addUniformVector("screenSize", storage->lightingPass.getSize());
      storage->ambientShader->addUniformFloat("intensity", storage->currentEnvironment->getIntensity());
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
      storage->directionalShaderShadowed->addUniformVector("screenSize", storage->lightingPass.getSize());
      // Camera properties.
      storage->directionalShaderShadowed->addUniformVector("camera.position", storage->sceneCam->getCamPos());
      storage->directionalShaderShadowed->addUniformMatrix("camera.cameraView", storage->sceneCam->getViewMatrix(), GL_FALSE);

      // Screen size.
      storage->directionalShader->addUniformVector("screenSize", storage->lightingPass.getSize());
      // Camera properties.
      storage->directionalShader->addUniformVector("camera.position", storage->sceneCam->getCamPos());
      storage->directionalShader->addUniformMatrix("camera.cameraView", storage->sceneCam->getViewMatrix(), GL_FALSE);

      // Set the shadow map uniforms.
      if (storage->hasCascades)
      {
        for (unsigned int i = 0; i < NUM_CASCADES; i++)
        {
          storage->directionalShaderShadowed->addUniformMatrix(
            (std::string("lightVP[") + std::to_string(i) + std::string("]")).c_str(),
            storage->cascades[i], GL_FALSE);
          storage->directionalShaderShadowed->addUniformFloat(
            (std::string("cascadeSplits[") + std::to_string(i) + std::string("]")).c_str(),
            storage->cascadeSplits[i]);
          storage->directionalShaderShadowed->addUniformFloat("lightBleedReduction", state->bleedReduction);
          storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Colour0, i + 7);
        }
      }

      for (auto& light : storage->directionalQueue)
      {
        if (light.castShadows && light.primaryLight)
        {
          // Light properties.
          storage->directionalShaderShadowed->addUniformVector("lDirection", light.direction);
          storage->directionalShaderShadowed->addUniformVector("lColour", light.colour);
          storage->directionalShaderShadowed->addUniformFloat("lIntensity", light.intensity);

          draw(&storage->fsq, storage->directionalShaderShadowed);
        }
        else
        {
          // Light properties.
          storage->directionalShader->addUniformVector("lDirection", light.direction);
          storage->directionalShader->addUniformVector("lColour", light.colour);
          storage->directionalShader->addUniformFloat("lIntensity", light.intensity);

          draw(&storage->fsq, storage->directionalShader);
        }
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
      storage->gBuffer.blitzToOther(storage->lightingPass, FBOTargetParam::Depth);
      drawEnvironment();

      storage->lightingPass.unbind();

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      stats->lightFrametime = elapsed.count() * 1000.0f;
    }

    //--------------------------------------------------------------------------
    // Post processing pass. TODO: Move most of these to the editor window and a
    // separate scene renderer?
    //--------------------------------------------------------------------------
    void
    postProcessPass(Shared<FrameBuffer> frontBuffer)
    {
      auto start = std::chrono::steady_clock::now();

      frontBuffer->clear();
      frontBuffer->bind();
      frontBuffer->setViewport();

      //------------------------------------------------------------------------
      // HDR post processing pass. Also streams the entity IDs to the editor
      // buffer.
      //------------------------------------------------------------------------
      storage->hdrPostShader->addUniformVector("screenSize", frontBuffer->getSize());
      storage->lightingPass.bindTextureID(FBOTargetParam::Colour0, 0);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour4, 1);

      draw(&storage->fsq, storage->hdrPostShader);

      RendererCommands::enable(RendererFunction::Blending);
      RendererCommands::blendEquation(BlendEquation::Additive);
      RendererCommands::blendFunction(BlendFunction::One, BlendFunction::One);
      RendererCommands::disable(RendererFunction::DepthTest);

      //------------------------------------------------------------------------
      // Grid post processing pass for the editor.
      //------------------------------------------------------------------------
      if (state->drawGrid)
      {
        storage->gridShader->addUniformMatrix("invViewProj", glm::inverse(storage->sceneCam->getProjMatrix() * storage->sceneCam->getViewMatrix()), GL_FALSE);
        storage->gridShader->addUniformMatrix("viewProj", storage->sceneCam->getProjMatrix() * storage->sceneCam->getViewMatrix(), GL_FALSE);
        storage->gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
        draw(&storage->fsq, storage->gridShader);
      }

      //------------------------------------------------------------------------
      // Edge detection post processing pass. Draws an outline around the
      // selected entity.
      //------------------------------------------------------------------------
      if (storage->drawEdge)
      {
        storage->outlineShader->addUniformVector("screenSize", frontBuffer->getSize());
        storage->gBuffer.bindAttachment(FBOTargetParam::Colour4, 0);

        draw(&storage->fsq, storage->outlineShader);
      }
      RendererCommands::enable(RendererFunction::DepthTest);
      RendererCommands::disable(RendererFunction::Blending);

      frontBuffer->unbind();

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      stats->postFramtime = elapsed.count() * 1000.0f;
    }
  }
}
