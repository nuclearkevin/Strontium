#include "Graphics/Renderer.h"

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
    init(const uint width, const uint height)
    {
      // Initialize OpenGL parameters.
      RendererCommands::enable(RendererFunction::DepthTest);
      RendererCommands::enable(RendererFunction::CubeMapSeamless);

      // Setup the various storage structs.
      storage = new RendererStorage();
      state = new RendererState();
      stats = new RendererStats();

      storage->width = width;
      storage->height = height;

      // Prepare bloom settings.
      Texture2DParams bloomParams = Texture2DParams();
      bloomParams.sWrap = TextureWrapParams::ClampEdges;
      bloomParams.tWrap = TextureWrapParams::ClampEdges;
      bloomParams.internal = TextureInternalFormats::RGBA16f;
      bloomParams.dataType = TextureDataType::Floats;

      float powOf2 = 2.0f;
      for (unsigned int i = 0; i < MAX_NUM_BLOOM_MIPS; i++)
      {
        storage->downscaleBloomTex[i].setSize((uint) ((float) width) / powOf2, (uint) ((float) height) / powOf2, 4);
        storage->downscaleBloomTex[i].setParams(bloomParams);
        storage->downscaleBloomTex[i].initNullTexture();

        storage->upscaleBloomTex[i].setSize((uint) ((float) width) / powOf2, (uint) ((float) height) / powOf2, 4);
        storage->upscaleBloomTex[i].setParams(bloomParams);
        storage->upscaleBloomTex[i].initNullTexture();

        powOf2 *= 2.0f;
      }

      powOf2 = 2.0f;
      for (unsigned int i = 0; i < MAX_NUM_BLOOM_MIPS - 1; i++)
      {
        storage->bufferBloomTex[i].setSize((uint)((float)width) / powOf2, (uint)((float)height) / powOf2, 4);
        storage->bufferBloomTex[i].setParams(bloomParams);
        storage->bufferBloomTex[i].initNullTexture();
        powOf2 *= 2.0f;
      }

      // Init the texture for the screen-space godrays. Use same params as bloom.
      storage->downsampleLightshaft.setSize(width / 2, height / 2, 4);
      storage->downsampleLightshaft.setParams(bloomParams);
      storage->downsampleLightshaft.initNullTexture();
      storage->halfResBuffer1.setSize(width / 2, height / 2, 4);
      storage->halfResBuffer1.setParams(bloomParams);
      storage->halfResBuffer1.initNullTexture();

      // Prepare the shadow buffers.
      auto dSpec = FBOCommands::getDefaultDepthSpec();
      auto vSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
      vSpec.internal = TextureInternalFormats::RGBA32f;
      vSpec.format = TextureFormats::RGBA;
      for (unsigned int i = 0; i < NUM_CASCADES; i++)
      {
        storage->shadowBuffer[i].resize(state->cascadeSize, state->cascadeSize);
        storage->shadowBuffer[i].attachTexture2D(vSpec);
        storage->shadowBuffer[i].attachTexture2D(dSpec);
        storage->shadowBuffer[i].setClearColour(glm::vec4(1.0f));
      }
      storage->shadowEffectsBuffer.resize(state->cascadeSize, state->cascadeSize);
      storage->shadowEffectsBuffer.attachTexture2D(vSpec);
      storage->shadowEffectsBuffer.attachTexture2D(dSpec);
      storage->shadowEffectsBuffer.setClearColour(glm::vec4(1.0f));
      storage->hasCascades = false;

      storage->gBuffer.resize(width, height);

      // The lighting pass framebuffer.
      storage->lightingPass.resize(width, height);
      auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
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
    begin(uint width, uint height, const Camera &sceneCamera)
    {
      storage->sceneCam = sceneCamera;
      storage->camFrustum = buildCameraFrustum(sceneCamera);

      // Resize the framebuffer at the start of a frame, if required.
      storage->drawEdge = false;

      // Update the frame.
      state->currentFrame++;
      if (state->currentFrame == 6)
        state->currentFrame = 0;

      // Resize any fullscreen textures.
      if (storage->width != width || storage->height != height)
      {
        storage->gBuffer.resize(width, height);
        storage->lightingPass.resize(width, height);

        float powOf2 = 2.0f;
        for (unsigned int i = 0; i < MAX_NUM_BLOOM_MIPS; i++)
        {
          storage->downscaleBloomTex[i].setSize((uint) ((float) width) / powOf2, (uint) ((float) height) / powOf2, 4);
          storage->downscaleBloomTex[i].initNullTexture();

          storage->upscaleBloomTex[i].setSize((uint) ((float) width) / powOf2, (uint) ((float) height) / powOf2, 4);
          storage->upscaleBloomTex[i].initNullTexture();

          if (i < MAX_NUM_BLOOM_MIPS - 1)
          {
            storage->bufferBloomTex[i].setSize((uint) ((float) width) / powOf2, (uint) ((float) height) / powOf2, 4);
            storage->bufferBloomTex[i].initNullTexture();
          }
          powOf2 *= 2.0f;
        }

        storage->downsampleLightshaft.setSize(width / 2, height / 2, 4);
        storage->downsampleLightshaft.initNullTexture();
        storage->halfResBuffer1.setSize(width / 2, height / 2, 4);
        storage->halfResBuffer1.initNullTexture();

        storage->width = width;
        storage->height = height;
      }

      // Clear the shadow pass FBOs.
      for (unsigned int i = 0; i < NUM_CASCADES; i++)
        storage->shadowBuffer[i].clear();

      // Clear the lighting pass FBO.
      storage->lightingPass.clear();

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

      // Clear the render queues.
      storage->staticRenderQueue.clear();
      storage->dynamicRenderQueue.clear();
    }

    void
    end(Shared<FrameBuffer> frontBuffer)
    {
      geometryPass();

      shadowPass();

      lightingPass();

      postProcessPass(frontBuffer);
    }

    // Draw the data to the screen.
    void
    draw(VertexArray* data, Shader* program)
    {
      data->bind();
      program->bind();

      RendererCommands::drawElements(PrimativeType::Triangle, data->numToRender());

      data->unbind();
      program->unbind();
    }

    // Draw an environment map to the screen. Draws all the submeshes associated
    // with the cube model.
    void
    drawEnvironment()
    {
      RendererCommands::depthFunction(DepthFunctions::LEq);
      storage->currentEnvironment->configure();

      storage->camBuffer.bindToPoint(0);
      storage->blankVAO.bind();
      ShaderCache::getShader("skybox")->bind();
      RendererCommands::drawArrays(PrimativeType::Triangle, 0, 36);

      RendererCommands::depthFunction(DepthFunctions::Less);
    }

    void
    submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
           float id, bool drawSelectionMask)
    {
      // Cull the model early.
      glm::vec3 min = data->getMinPos();
      glm::vec3 max = data->getMaxPos();
      glm::vec3 center = (min + max) / 2.0f;

      if (boundingBoxInFrustum(storage->camFrustum, min, max, model) && state->frustumCull)
        storage->staticRenderQueue.emplace_back(data, &materials, model, id,
                                                drawSelectionMask);
      else if (!state->frustumCull)
        storage->staticRenderQueue.emplace_back(data, &materials, model, id,
                                                drawSelectionMask);

      storage->staticShadowQueue.emplace_back(data, model);
    }

    void submit(Model* data, Animator* animation, ModelMaterial &materials,
                const glm::mat4 &model, float id, bool drawSelectionMask)
    {
      // Cull the model early.
      glm::vec3 min = data->getMinPos();
      glm::vec3 max = data->getMaxPos();

      if (boundingBoxInFrustum(storage->camFrustum, min, max, model) && state->frustumCull)
        storage->dynamicRenderQueue.emplace_back(data, animation, &materials,
                                                 model, id, drawSelectionMask);
      else if (!state->frustumCull)
        storage->dynamicRenderQueue.emplace_back(data, animation, &materials,
                                                 model, id, drawSelectionMask);

      storage->dynamicShadowQueue.emplace_back(data, animation, model);
    }

    void
    submit(DirectionalLight light, const glm::mat4 &model)
    {
      auto invTrans = glm::transpose(glm::inverse(model));
      DirectionalLight temp = light;
      temp.direction = -1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));

      storage->directionalQueue.push_back(temp);
      stats->numDirLights++;
    }

    void
    submit(PointLight light, const glm::mat4 &model)
    {
      PointLight temp = light;
      temp.positionRadius = glm::vec4(glm::vec3(model * glm::vec4(glm::vec3(temp.positionRadius), 1.0f)), temp.positionRadius.w);

      storage->pointQueue.push_back(temp);
      stats->numPointLights++;
    }

    void
    submit(SpotLight light, const glm::mat4 &model)
    {
      auto invTrans = glm::transpose(glm::inverse(model));
      SpotLight temp = light;
      temp.direction = -1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
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
      storage->camBuffer.bindToPoint(0);
      storage->camBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(storage->sceneCam.view));
      storage->camBuffer.setData(sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(storage->sceneCam.projection));
      storage->camBuffer.setData(2 * sizeof(glm::mat4), sizeof(glm::vec3), &(storage->sceneCam.position.x));

      // Start the geometry pass.
      storage->gBuffer.beginGeoPass();

      storage->transformBuffer.bindToPoint(2);
      storage->editorBuffer.bindToPoint(3);

      // Static geometry pass.
      Shader* program = ShaderCache::getShader("geometry_pass_shader");
      for (auto& drawable : storage->staticRenderQueue)
      {
        auto& [data, materials, transform, id, drawSelectionMask] = drawable;
        for (auto& submesh : data->getSubmeshes())
        {
          // Cull the submesh if it isn't in the frustum.
          glm::vec3 min = submesh.getMinPos();
          glm::vec3 max = submesh.getMaxPos();

          if (!boundingBoxInFrustum(storage->camFrustum, min, max, transform) && state->frustumCull)
            continue;

          Material* material = materials->getMaterial(submesh.getName());
          if (!material)
            continue;

          storage->transformBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(transform));

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

      // Dynamic geometry pass.
      program = ShaderCache::getShader("dynamic_geometry_pass");
      storage->boneBuffer.bindToPoint(4);
      for (auto& drawable : storage->dynamicRenderQueue)
      {
        auto& [data, animation, materials, transform, id, drawSelectionMask] = drawable;

        auto& bones = animation->getFinalBoneTransforms();
        storage->boneBuffer.setData(0, bones.size() * sizeof(glm::mat4),
                                    bones.data());

        for (auto& submesh : data->getSubmeshes())
        {
          // Cull the submesh if it isn't in the frustum.
          glm::vec3 min = submesh.getMinPos();
          glm::vec3 max = submesh.getMaxPos();

          if (!boundingBoxInFrustum(storage->camFrustum, min, max, transform) && state->frustumCull)
            continue;

          Material* material = materials->getMaterial(submesh.getName());
          if (!material)
          {
            continue;
          }

          storage->transformBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(transform));

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

          material->configureDynamic(program);

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
      stats->geoFrametime += elapsed.count() * 1000.0f;
    }

    //--------------------------------------------------------------------------
    // Deferred shadow mapping pass. Cascaded shadows for a "primary light".
    //--------------------------------------------------------------------------
    // TODO: Soft shadow quality settings (hard shadows, Low, medium, high, ultra).
    // Low is a simple box blur, medium->ultra are gaussian with different number
    // of taps. Hard shadows are regular shadow maps with zero prefiltering.
    void
    shadowPass()
    {
      auto start = std::chrono::steady_clock::now();

      //------------------------------------------------------------------------
      // Directional light shadow cascade calculations:
      //------------------------------------------------------------------------
      // https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
      //------------------------------------------------------------------------
      // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps
      //------------------------------------------------------------------------
      const float near = storage->sceneCam.near;
      const float far = storage->sceneCam.far;

      glm::mat4 camInvVP = storage->sceneCam.invViewProj;

      Frustum lightCullingFrustums[NUM_CASCADES];

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
      for (auto& pair : storage->staticShadowQueue)
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
          // https://docs.microsoft.com/en-ca/windows/win32/dxtecharts/common-
          // techniques-to-improve-shadow-depth-maps?redirectedfrom=MSDN
          //--------------------------------------------------------------------
          glm::mat4 lightVP = cascadeProjMatrix[i] * cascadeViewMatrix[i];
          glm::vec4 shadowOrigin = glm::vec4(glm::vec3(0.0f), 1.0f);
          shadowOrigin = lightVP * shadowOrigin;
          float storedShadowW = shadowOrigin.w;
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
          lightCullingFrustums[i] = buildCameraFrustum(storage->cascades[i], -lightDir);

          previousCascadeDistance = cascadeSplits[i];

          storage->cascadeSplits[i] = near + (cascadeSplits[i] * (far - near));
        }
      }

      // Actual shadow pass.
      Shader* horizontalShadowBlur = ShaderCache::getShader("gaussian_hori");
      Shader* verticalShadowBlur = ShaderCache::getShader("gaussian_vert");

      storage->transformBuffer.bindToPoint(2);

      if (storage->hasCascades)
      {
        for (unsigned int i = 0; i < NUM_CASCADES; i++)
        {
          storage->shadowBuffer[i].bind();
          storage->shadowBuffer[i].setViewport();

          storage->cascadeShadowPassBuffer.bindToPoint(6);
          storage->cascadeShadowPassBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(storage->cascades[i]));

          // Static shadow pass.
          Shader* program = ShaderCache::getShader("static_shadow_shader");
          for (auto& [model, transform] : storage->staticShadowQueue)
          {
              storage->transformBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(transform));

              for (auto& submesh : model->getSubmeshes())
              {
                  // Cull the submesh if it isn't in the frustum.
                  glm::vec3 min = submesh.getMinPos();
                  glm::vec3 max = submesh.getMaxPos();

                  if (!boundingBoxInFrustum(lightCullingFrustums[i], min, max, transform) && state->frustumCull)
                      continue;

                  if (submesh.hasVAO())
                      Renderer3D::draw(submesh.getVAO(), program);
                  else
                  {
                      submesh.generateVAO();
                      Renderer3D::draw(submesh.getVAO(), program);
                  }
              }
          }

          // Dynamic shadow pass.
          program = ShaderCache::getShader("dynamic_shadow_shader");
          for (auto& drawable : storage->dynamicShadowQueue)
          {
            auto& [data, animation, transform] = drawable;
            storage->transformBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(transform));

            auto& bones = animation->getFinalBoneTransforms();
            storage->boneBuffer.setData(0, bones.size() * sizeof(glm::mat4),
                bones.data());

            for (auto& submesh : data->getSubmeshes())
            {
              // Cull the submesh if it isn't in the frustum.
              glm::vec3 min = submesh.getMinPos();
              glm::vec3 max = submesh.getMaxPos();

              if (!boundingBoxInFrustum(lightCullingFrustums[i], min, max, transform) && state->frustumCull)
                continue;

              if (submesh.hasVAO())
                Renderer3D::draw(submesh.getVAO(), program);
              else
              {
                submesh.generateVAO();
                Renderer3D::draw(submesh.getVAO(), program);
              }
            }
          }
        }

        if (state->directionalSettings.x == 2)
        {
          // Apply a 2-pass 9 tap Gaussian blur to the shadow map.
          RendererCommands::disableDepthMask();
          RendererCommands::disable(RendererFunction::DepthTest);
          for (unsigned int i = 0; i < NUM_CASCADES; i++)
          {
            // First pass (horizontal) is FBO attachment -> temp FBO.
            storage->shadowEffectsBuffer.bind();
            storage->shadowEffectsBuffer.setViewport();

            storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Colour0, 0);
            storage->blankVAO.bind();
            horizontalShadowBlur->bind();
            RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);

            storage->shadowEffectsBuffer.unbind();

            // Second pass (vertical) is temp FBO -> FBO attachment.
            storage->shadowBuffer[i].bind();
            storage->shadowBuffer[i].setViewport();

            storage->shadowEffectsBuffer.bindTextureID(FBOTargetParam::Colour0, 0);
            storage->blankVAO.bind();
            verticalShadowBlur->bind();
            RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);

            storage->shadowBuffer[i].unbind();
          }
          RendererCommands::enable(RendererFunction::DepthTest);
          RendererCommands::enableDepthMask();
        }
      }

      storage->staticShadowQueue.clear();
      storage->dynamicShadowQueue.clear();

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      stats->shadowFrametime += elapsed.count() * 1000.0f;
    }

    //--------------------------------------------------------------------------
    // Deferred lighting pass.
    //--------------------------------------------------------------------------
    void
    lightingPass()
    {
      auto start = std::chrono::steady_clock::now();

      RendererCommands::disable(RendererFunction::DepthTest);
      storage->lightingPass.bind();
      storage->lightingPass.setViewport();

      //------------------------------------------------------------------------
      // Ambient lighting subpass.
      //------------------------------------------------------------------------
      Shader* ambientShader = ShaderCache::getShader("deferred_ambient");
      // Environment maps.
      storage->currentEnvironment->bind(MapType::Irradiance, 0);
      storage->currentEnvironment->bind(MapType::Prefilter, 1);
      storage->currentEnvironment->bindBRDFLUT(2);
      // Gbuffer textures.
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour0, 3);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour1, 4);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour2, 5);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour3, 6);

      auto sizeIntensity = glm::vec3(0.0f);
      auto screenSize = storage->lightingPass.getSize();
      sizeIntensity.x = screenSize.x;
      sizeIntensity.y = screenSize.y;
      sizeIntensity.z = storage->currentEnvironment->getIntensity();
      storage->ambientPassBuffer.bindToPoint(4);
      storage->ambientPassBuffer.setData(0, sizeof(glm::vec3), &sizeIntensity.x);

      storage->blankVAO.bind();
      ambientShader->bind();
      RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);

      //------------------------------------------------------------------------
      // Directional lighting subpass.
      //------------------------------------------------------------------------
      Shader* directionalLightShadowed = ShaderCache::getShader("deferred_directional_shadowed");
      Shader* directionalLight = ShaderCache::getShader("deferred_directional");
      RendererCommands::enable(RendererFunction::Blending);
      RendererCommands::blendEquation(BlendEquation::Additive);
      RendererCommands::blendFunction(BlendFunction::One, BlendFunction::One);

      storage->directionalPassBuffer.bindToPoint(5);
      storage->directionalPassBuffer.setData(2 * sizeof(glm::vec4), sizeof(glm::ivec4), &(state->directionalSettings.x));

      // Set the shadow map uniforms.
      if (storage->hasCascades)
      {
        storage->cascadeShadowBuffer.bindToPoint(7);
        for (unsigned int i = 0; i < NUM_CASCADES; i++)
        {
          storage->cascadeShadowBuffer.setData(i * sizeof(glm::mat4), sizeof(glm::mat4),
                                                glm::value_ptr(storage->cascades[i]));
          storage->cascadeShadowBuffer.setData(NUM_CASCADES * sizeof(glm::mat4)
                                                + i * sizeof(glm::vec4), sizeof(float),
                                                &storage->cascadeSplits[i]);
          if (state->directionalSettings.x == 2)
            storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Colour0, i + 7);
          else
            storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Depth, i + 7);
        }
        storage->cascadeShadowBuffer.setData(NUM_CASCADES * sizeof(glm::mat4)
                                              + NUM_CASCADES * sizeof(glm::vec4),
                                              sizeof(glm::vec4),
                                              &(state->shadowParams.x));
      }

      for (auto& light : storage->directionalQueue)
      {
        auto dirColourIntensity = glm::vec4(0.0f);
        dirColourIntensity.x = light.colour.x;
        dirColourIntensity.y = light.colour.y;
        dirColourIntensity.z = light.colour.z;
        dirColourIntensity.w = light.intensity;
        storage->directionalPassBuffer.setData(0, sizeof(glm::vec4), &dirColourIntensity.x);

        auto dirDirection = glm::vec4(0.0f);
        dirDirection.x = light.direction.x;
        dirDirection.y = light.direction.y;
        dirDirection.z = light.direction.z;
        storage->directionalPassBuffer.setData(sizeof(glm::vec4), sizeof(glm::vec4), &dirDirection.x);

        if (light.castShadows && light.primaryLight)
        {
           for (unsigned int i = 0; i < NUM_CASCADES; i++)
             storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Depth, i + 7);

           state->postProcessSettings.w = (uint)(storage->hasCascades && state->enableSkyshafts);
           storage->postProcessSettings.bindToPoint(1);
           storage->postProcessSettings.setData(0, sizeof(glm::mat4), glm::value_ptr(storage->sceneCam.invViewProj));
           storage->postProcessSettings.setData(2 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), sizeof(glm::ivec4), &state->postProcessSettings.x);

          // Launch the godray compute shaders.
          if (state->enableSkyshafts)
          {
            storage->gBuffer.bindAttachment(FBOTargetParam::Depth, 1);
            storage->lightShaftSettingsBuffer.bindToPoint(0);
            storage->lightShaftSettingsBuffer.setData(0, sizeof(glm::vec4),
                                                      &(state->mieScatIntensity.x));
            storage->lightShaftSettingsBuffer.setData(sizeof(glm::vec4), sizeof(glm::vec4),
                                                      &(state->mieAbsDensity.x));   

            storage->halfResBuffer1.bindAsImage(0, 0, ImageAccessPolicy::Write);
            uint iWidth = (uint) glm::ceil(storage->downsampleLightshaft.width / 32.0f);
            uint iHeight = (uint) glm::ceil(storage->downsampleLightshaft.height / 32.0f);
            ShaderCache::getShader("screen_space_godrays")->launchCompute(iWidth, iHeight, 1);
            Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

            storage->halfResBuffer1.bindAsImage(0, 0, ImageAccessPolicy::Read);
            storage->downsampleLightshaft.bindAsImage(1, 0, ImageAccessPolicy::Write);
            ShaderCache::getShader("bilateral_blur")->launchCompute(iWidth, iHeight, 1);
            Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
          }

          for (unsigned int i = 0; i < NUM_CASCADES; i++)
          {
            if (state->directionalSettings.x == 2)
                storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Colour0, i + 7);
            else
                storage->shadowBuffer[i].bindTextureID(FBOTargetParam::Depth, i + 7);
          }

          storage->downsampleLightshaft.bind(1);

          // Launch the primary shadowed directional light pass.
          storage->blankVAO.bind();
          directionalLightShadowed->bind();
          RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);
        }
        else
        {
          storage->blankVAO.bind();
          directionalLight->bind();
          RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);
        }
      }

      storage->directionalQueue.clear();

      //------------------------------------------------------------------------
      // Point lighting subpass.
      //------------------------------------------------------------------------
      Shader* pointLight = ShaderCache::getShader("deferred_point");
      storage->pointPassBuffer.bindToPoint(5);
      for (auto& light : storage->pointQueue)
      {
        storage->pointPassBuffer.setData(0, sizeof(PointLight), &light);

        storage->blankVAO.bind();
        pointLight->bind();
        RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);
      }
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
      stats->lightFrametime += elapsed.count() * 1000.0f;
    }

    //--------------------------------------------------------------------------
    // Post processing pass. TODO: Move most of these to the editor window and a
    // separate scene renderer?
    //--------------------------------------------------------------------------
    void
    postProcessPass(Shared<FrameBuffer> frontBuffer)
    {
      auto start = std::chrono::steady_clock::now();

      //------------------------------------------------------------------------
      // Bloom pass.
      //------------------------------------------------------------------------
      if (state->enableBloom)
      {
        // Bloom shaders.
        auto downsample = ShaderCache::getShader("bloom_downsample");
        auto upsample = ShaderCache::getShader("bloom_upsample");
        auto upsampleBlend = ShaderCache::getShader("bloom_upsample_blend");

        // Prefilter + downsample using the lighting buffer as the source.
        storage->bloomSettingsBuffer.bindToPoint(3);
        auto bloomSettings = glm::vec4(state->bloomThreshold, state->bloomThreshold - state->bloomKnee,
                                       2.0f * state->bloomKnee, 0.25 / state->bloomKnee);
        storage->bloomSettingsBuffer.setData(0, sizeof(glm::vec4), &bloomSettings.x);
        storage->bloomSettingsBuffer.setData(sizeof(glm::vec4), sizeof(float), &state->bloomRadius);

        storage->lightingPass.getAttachment(FBOTargetParam::Colour0)
               ->bindAsImage(0, 0, ImageAccessPolicy::Read);
        storage->downscaleBloomTex[0].bindAsImage(1, 0, ImageAccessPolicy::Write);
        glm::ivec3 invoke = glm::ivec3(glm::ceil(((float) storage->downscaleBloomTex[0].width) / 32.0f),
                                       glm::ceil(((float) storage->downscaleBloomTex[0].height) / 32.0f),
                                       1);
        ShaderCache::getShader("bloom_prefilter")->launchCompute(invoke.x, invoke.y, invoke.z);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

        // Continuously downsample the bloom texture to make an image pyramid.
        for (unsigned int i = 1; i < MAX_NUM_BLOOM_MIPS; i++)
        {
          storage->downscaleBloomTex[i - 1].bindAsImage(0, 0, ImageAccessPolicy::Read);
          storage->downscaleBloomTex[i].bindAsImage(1, 0, ImageAccessPolicy::Write);
          invoke = glm::ivec3(glm::ceil(((float) storage->downscaleBloomTex[i].width) / 32.0f),
                              glm::ceil(((float) storage->downscaleBloomTex[i].height) / 32.0f),
                              1);
          downsample->launchCompute(invoke.x, invoke.y, invoke.z);
          Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
        }

        // Blur each of the mips.
        for (unsigned int i = 0; i < MAX_NUM_BLOOM_MIPS - 1; i++)
        {
          storage->downscaleBloomTex[i].bindAsImage(0, 0, ImageAccessPolicy::Read);
          storage->bufferBloomTex[i].bindAsImage(1, 0, ImageAccessPolicy::Write);
          invoke = glm::ivec3(glm::ceil(((float) storage->bufferBloomTex[i].width) / 32.0f),
                              glm::ceil(((float) storage->bufferBloomTex[i].height) / 32.0f),
                              1);
          upsample->launchCompute(invoke.x, invoke.y, invoke.z);
          Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
        }

        // Copy and blur the last mip of the downsampling pyramid into the last
        // mip of the upsampling pyramid.
        storage->downscaleBloomTex[MAX_NUM_BLOOM_MIPS - 1].bindAsImage(0, 0, ImageAccessPolicy::Read);
        storage->upscaleBloomTex[MAX_NUM_BLOOM_MIPS - 1].bindAsImage(1, 0, ImageAccessPolicy::Write);
        invoke = glm::ivec3(glm::ceil(((float) storage->bufferBloomTex[MAX_NUM_BLOOM_MIPS - 2].width) / 32.0f),
                            glm::ceil(((float) storage->bufferBloomTex[MAX_NUM_BLOOM_MIPS - 2].height) / 32.0f),
                            1);
        upsample->launchCompute(invoke.x, invoke.y, invoke.z);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

        // Blend the previous mips together with a blur on the previous mip.
        for (unsigned int i = MAX_NUM_BLOOM_MIPS - 1; i > 0; i--)
        {
          storage->upscaleBloomTex[i].bindAsImage(0, 0, ImageAccessPolicy::Read);
          storage->bufferBloomTex[i - 1].bindAsImage(1, 0, ImageAccessPolicy::Read);
          storage->upscaleBloomTex[i - 1].bindAsImage(2, 0, ImageAccessPolicy::Write);
          invoke = glm::ivec3(glm::ceil(((float) storage->upscaleBloomTex[i - 1].width) / 32.0f),
                              glm::ceil(((float) storage->upscaleBloomTex[i - 1].height) / 32.0f),
                              1);
          upsampleBlend->launchCompute(invoke.x, invoke.y, invoke.z);
          Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
        }
      }

      // Prep the front buffer.
      frontBuffer->bind();
      frontBuffer->setViewport();

      //------------------------------------------------------------------------
      // Generalized post processing shader.
      //------------------------------------------------------------------------
      // Prepare the post processing buffer.
      storage->postProcessSettings.bindToPoint(1);
      auto viewProj = storage->sceneCam.projection * storage->sceneCam.view;
      auto invViewProj = storage->sceneCam.invViewProj;
      auto camPos = storage->sceneCam.position;
      auto screenSize = frontBuffer->getSize();
      auto data0 = glm::vec4(camPos.x, camPos.y, camPos.z, screenSize.x);
      auto data1 = glm::vec4(screenSize.y, state->gamma, state->bloomIntensity, 0.0f);

      state->postProcessSettings.y = (uint) state->enableBloom;
      state->postProcessSettings.z = (uint) state->enableFXAA;

      storage->postProcessSettings.setData(0, sizeof(glm::mat4), glm::value_ptr(invViewProj));
      storage->postProcessSettings.setData(sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(viewProj));
      storage->postProcessSettings.setData(2 * sizeof(glm::mat4), sizeof(glm::vec4), &data0.x);
      storage->postProcessSettings.setData(2 * sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec4), &data1.x);
      storage->postProcessSettings.setData(2 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), sizeof(glm::ivec4), &state->postProcessSettings.x);

      storage->lightingPass.bindTextureID(FBOTargetParam::Colour0, 0);
      storage->gBuffer.bindAttachment(FBOTargetParam::Colour4, 1);
      storage->upscaleBloomTex[0].bind(2);

      storage->blankVAO.bind();
      ShaderCache::getShader("post_processing")->bind();
      RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);

      RendererCommands::enable(RendererFunction::Blending);
      RendererCommands::blendEquation(BlendEquation::Additive);
      RendererCommands::blendFunction(BlendFunction::One, BlendFunction::One);
      RendererCommands::disable(RendererFunction::DepthTest);

      //------------------------------------------------------------------------
      // Grid post processing pass for the editor.
      //------------------------------------------------------------------------
      if (state->drawGrid)
      {
        storage->gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
        storage->blankVAO.bind();
        ShaderCache::getShader("grid")->bind();
        RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);
      }

      //------------------------------------------------------------------------
      // Edge detection post processing pass. Draws an outline around the
      // selected entity.
      //------------------------------------------------------------------------
      if (storage->drawEdge)
      {
        storage->gBuffer.bindAttachment(FBOTargetParam::Colour4, 0);
        storage->blankVAO.bind();
        ShaderCache::getShader("outline")->bind();
        RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);
      }
      RendererCommands::enable(RendererFunction::DepthTest);
      RendererCommands::disable(RendererFunction::Blending);

      frontBuffer->unbind();

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      stats->postFramtime += elapsed.count() * 1000.0f;
    }
  }
}
