#include "Graphics/RenderPasses/ShadowPass.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RendererCommands.h"

namespace Strontium
{
  ShadowPass::ShadowPass(Renderer3D::GlobalRendererData* globalRendererData)
	: RenderPass(&this->passData, globalRendererData, { nullptr })
    , timer(5)
  { }
  
  ShadowPass::~ShadowPass()
  { }
  
  void 
  ShadowPass::onInit()
  {
    this->passData.staticShadow = ShaderCache::getShader("static_shadow_shader");
    this->passData.dynamicShadow = ShaderCache::getShader("dynamic_shadow_shader");

    auto dSpec = Texture2D::getDefaultDepthParams();
    dSpec.sWrap = TextureWrapParams::ClampEdges;
    dSpec.tWrap = TextureWrapParams::ClampEdges;

    auto depthAttachment = FBOAttachment(FBOTargetParam::Depth, FBOTextureParam::Texture2D,
                                         dSpec.internal, dSpec.format, dSpec.dataType);

    for (uint i = 0; i < NUM_CASCADES; i++)
    {
      this->passData.shadowBuffers[i].resize(this->passData.shadowMapRes, this->passData.shadowMapRes);
      this->passData.shadowBuffers[i].attach(dSpec, depthAttachment);
    }
  }
  
  void 
  ShadowPass::updatePassData()
  {
    for (uint i = 0; i < NUM_CASCADES; i++)
    {
      if (this->passData.shadowMapRes != 
          static_cast<uint>(this->passData.shadowBuffers[i].getSize().x))
        this->passData.shadowBuffers[i].resize(this->passData.shadowMapRes, this->passData.shadowMapRes);
    }
  }
  
  RendererDataHandle 
  ShadowPass::requestRendererData()
  {
    return -1;
  }

  void 
  ShadowPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  ShadowPass::onRendererBegin(uint width, uint height)
  {
    // Clear the shadow buffers to prevent a stall.
    for (uint i = 0; i < NUM_CASCADES; i++)
      this->passData.shadowBuffers[i].clear();

    this->passData.castShadows = false;
    this->passData.hasCascades = false;

    this->passData.numUniqueEntities = 0u;
    this->passData.staticDrawList.clear();
    this->passData.dynamicDrawList.clear();
    this->passData.minPos = glm::vec3(std::numeric_limits<float>::max());
    this->passData.maxPos = glm::vec3(std::numeric_limits<float>::min());

	// Clear the statistics.
    this->passData.numDrawCalls = 0u;
    this->passData.numInstances = 0u;
    this->passData.numTrianglesSubmitted = 0u;
    this->passData.numTrianglesDrawn = 0u;
  }
  
  void 
  ShadowPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    // Compute the cascade data.
    this->computeShadowData();

    if (!this->passData.hasCascades)
      return;

    // Upload the cached data for both static and dynamic geometry.
    if (this->passData.transformBuffer.size() != (sizeof(glm::mat4) * this->passData.numUniqueEntities))
      this->passData.transformBuffer.resize(sizeof(glm::mat4) * this->passData.numUniqueEntities, BufferType::Dynamic);
    
    uint bufferPointer = 0;
    for (auto& drawCommand : this->passData.staticDrawList)
    {
      this->passData.transformBuffer.setData(bufferPointer, sizeof(glm::mat4) * drawCommand.instancedTransforms.size(),
                                              drawCommand.instancedTransforms.data());
      bufferPointer += sizeof(glm::mat4) * drawCommand.instancedTransforms.size();
    }
    for (auto& drawCommand : this->passData.dynamicDrawList)
    {
      this->passData.transformBuffer.setData(bufferPointer, sizeof(glm::mat4),
                                             &drawCommand.transform);
      bufferPointer += sizeof(glm::mat4);
    }

    // Run the Strontium render pipeline for each shadow cascade.
    for (uint i = 0; i < NUM_CASCADES; i++)
    {
      // Set the light-space data.
      struct LightSpaceBlockData
      {
        glm::mat4 lightViewProj;
      }
        lightSpaceBlock
      {
        this->passData.cascades[i]
      };
      this->passData.lightSpaceBuffer.setData(0, sizeof(LightSpaceBlockData), &lightSpaceBlock);

      // Begin the actual shadow pass for this cascade.
      this->passData.shadowBuffers[i].setViewport();
      this->passData.shadowBuffers[i].bind();
      this->passData.lightSpaceBuffer.bindToPoint(0);
      this->passData.perDrawUniforms.bindToPoint(1);

      this->passData.transformBuffer.bindToPoint(0);

      bufferPointer = 0;
      // Static geometry pass.
      this->passData.staticShadow->bind();
      for (auto& drawable : this->passData.staticDrawList)
      {
        // Set the index offset. 
        this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferPointer);
      
        auto vao = static_cast<VertexArray*>(drawable);
        vao->bind();
        RendererCommands::drawElementsInstanced(PrimativeType::Triangle, 
                                                vao->numToRender(), 
                                                drawable.instancedTransforms.size());
        vao->unbind();
        
        bufferPointer += drawable.instancedTransforms.size();
      
        // Record some statistics.
        this->passData.numDrawCalls++;
        this->passData.numInstances += drawable.instancedTransforms.size();
        this->passData.numTrianglesDrawn += (drawable.instancedTransforms.size() * vao->numToRender()) / 3;
      }
      
      // Dynamic geometry pass for skinned objects.
      // TODO: Improve this with compute shader skinning. 
      // Could probably get rid of this pass all together.
      this->passData.dynamicShadow->bind();
      this->passData.boneBuffer.bindToPoint(1);
      for (auto& drawable : this->passData.dynamicDrawList)
      {
        auto& bones = static_cast<Animator*>(drawable)->getFinalBoneTransforms();
        this->passData.boneBuffer.setData(0, bones.size() * sizeof(glm::mat4),
                                          bones.data());
        
        // Set the index offset. 
        this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferPointer);

        auto vao = static_cast<VertexArray*>(drawable);
        vao->bind();
        RendererCommands::drawElementsInstanced(PrimativeType::Triangle, 
                                                vao->numToRender(), 
                                                drawable.instanceCount);
        vao->unbind();
      
        bufferPointer += drawable.instanceCount;
      
        // Record some statistics.
        this->passData.numDrawCalls++;
        this->passData.numInstances += drawable.instanceCount;
        this->passData.numTrianglesDrawn += (drawable.instanceCount * vao->numToRender()) / 3;
      }
    }
    this->passData.dynamicShadow->unbind();
    this->passData.shadowBuffers[NUM_CASCADES - 1].unbind();
  }
  
  void 
  ShadowPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }
  
  void 
  ShadowPass::onShutdown()
  { }

  void 
  ShadowPass::submit(Model* data, const glm::mat4& model)
  { 
	for (auto& submesh : data->getSubmeshes())
	{
      VertexArray* vao = submesh.hasVAO() ? submesh.getVAO() : submesh.generateVAO();
      if (!vao)
        continue;
      
      // Compute the scene AABB.
      auto localTransform = model * submesh.getTransform();
      this->passData.minPos = glm::min(this->passData.minPos, glm::vec3(localTransform * glm::vec4(submesh.getMinPos(), 1.0f)));
      this->passData.maxPos = glm::max(this->passData.maxPos, glm::vec3(localTransform * glm::vec4(submesh.getMaxPos(), 1.0f)));

      // Record some statistics.
      this->passData.numTrianglesSubmitted += vao->numToRender() / 3;
    
      // Populate the draw list.
      this->passData.numUniqueEntities++;
      auto drawData = std::find_if(this->passData.staticDrawList.begin(), 
                                   this->passData.staticDrawList.end(),
                                   [vao](const ShadowStaticDrawData& data)
      {
        return data.primatives == vao;
      });
    
      if (drawData != this->passData.staticDrawList.end())
      {   
        // Cache the per-entity data.
        drawData->instancedTransforms.emplace_back(localTransform);
      }
      else 
      {
        this->passData.staticDrawList.emplace_back(vao);
        this->passData.staticDrawList.back().instancedTransforms.emplace_back(localTransform);
      }
	}
  }

  void 
  ShadowPass::submit(Model* data, Animator* animation, const glm::mat4 &model)
  {
    if (data->hasSkins())
    {
      // Skinned animated mesh, store the static transform.
      for (auto& submesh : data->getSubmeshes())
	  {    
        VertexArray* vao = submesh.hasVAO() ? submesh.getVAO() : submesh.generateVAO();
        if (!vao)
          continue;
        
        this->passData.minPos = glm::min(this->passData.minPos, glm::vec3(model * glm::vec4(submesh.getMinPos(), 1.0f)));
        this->passData.maxPos = glm::max(this->passData.maxPos, glm::vec3(model * glm::vec4(submesh.getMaxPos(), 1.0f)));

        // Record some statistics.
        this->passData.numTrianglesSubmitted += vao->numToRender() / 3;
    
        // Populate the dynamic draw list.
        this->passData.numUniqueEntities++;
        this->passData.dynamicDrawList.emplace_back(vao, animation, model);
	  }
    }
    else
    {
      // Unskinned animated mesh, store the rigged transform.
	  auto& bones = animation->getFinalUnSkinnedTransforms();
	  for (auto& submesh : data->getSubmeshes())
	  {
        VertexArray* vao = submesh.hasVAO() ? submesh.getVAO() : submesh.generateVAO();
        if (!vao)
          continue;
    
        auto localTransform = model * bones[submesh.getName()];
        this->passData.minPos = glm::min(this->passData.minPos, glm::vec3(localTransform * glm::vec4(submesh.getMinPos(), 1.0f)));
        this->passData.maxPos = glm::max(this->passData.maxPos, glm::vec3(localTransform * glm::vec4(submesh.getMaxPos(), 1.0f)));
        
        // Record some statistics.
        this->passData.numTrianglesSubmitted += vao->numToRender() / 3;
    
        // Populate the draw list.
        this->passData.numUniqueEntities++;
        auto drawData = std::find_if(this->passData.staticDrawList.begin(), 
                                     this->passData.staticDrawList.end(),
                                     [vao](const ShadowStaticDrawData& data)
        {
          return data.primatives == vao;
        });
        
        if (drawData != this->passData.staticDrawList.end())
        {
          // Cache the per-entity data.
          drawData->instancedTransforms.emplace_back(localTransform);
        }
        else 
        {
          // Cache the per-entity data.
          this->passData.staticDrawList.emplace_back(vao);
          this->passData.staticDrawList.back().instancedTransforms.emplace_back(localTransform);
        }
	  }
    }
  }

  void 
  ShadowPass::submitPrimary(const DirectionalLight &light, bool castShadows, 
                            const glm::mat4& model)
  {
    auto invTrans = glm::transpose(glm::inverse(model));
    DirectionalLight temp = light;
    temp.directionSize = glm::vec4(-1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), light.directionSize.w);

    this->passData.primaryLight = temp;
    this->passData.castShadows = castShadows;
  }

  void 
  ShadowPass::computeShadowData()
  {
    //------------------------------------------------------------------------
    // Directional light shadow cascade calculations:
    //------------------------------------------------------------------------
    // https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
    //------------------------------------------------------------------------
    // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps
    //------------------------------------------------------------------------
    const float near = this->globalBlock->sceneCam.near;
    const float far = this->globalBlock->sceneCam.far;

    glm::mat4 camInvVP = this->globalBlock->sceneCam.invViewProj;

    Frustum lightCullingFrustums[NUM_CASCADES];
    float cascadeSplits[NUM_CASCADES];

    const float clipRange = far - near;
    // Calculate the optimal cascade distances
    const float minZ = near;
    const float maxZ = near + clipRange;
    const float range = maxZ - minZ;
    const float ratio = maxZ / minZ;
    for (uint i = 0; i < NUM_CASCADES; i++)
    {
      const float p = (static_cast<float>(i) + 1.0f) / static_cast<float>(NUM_CASCADES);
      const float log = minZ * glm::pow(ratio, p);
      const float uniform = minZ + range * p;
      const float d = this->passData.cascadeLambda * (log - uniform) + uniform;
      cascadeSplits[i] = (d - near) / clipRange;
    }

    // Compute the scene AABB in world space. This fixes issues with objects
    // not being captured if they're out of the camera frustum (since they
    // still need to cast shadows).
    float sceneMaxRadius = glm::length(this->passData.minPos);
    sceneMaxRadius = glm::max(sceneMaxRadius, glm::length(this->passData.maxPos));

    // Compute the lightspace matrices for each light for each cascade.
    if (!this->passData.castShadows)
      return;

    this->passData.hasCascades = true;
    glm::vec3 lightDir = glm::normalize(glm::vec3(this->passData.primaryLight.directionSize));

    if (this->passData.hasCascades)
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
        radius = std::ceil(radius * 16.0f) / 16.0f;
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
        shadowOrigin = 0.5f * shadowOrigin * this->passData.shadowBuffers[i].getSize().x;

        glm::vec4 roundedShadowOrigin = glm::round(shadowOrigin);
        glm::vec4 roundedShadowOffset = roundedShadowOrigin - shadowOrigin;
        roundedShadowOffset = 2.0f * roundedShadowOffset / this->passData.shadowBuffers[i].getSize().x;
        roundedShadowOffset.z = 0.0f;
        roundedShadowOffset.w = 0.0f;

        glm::mat4 texelSpaceOrtho = cascadeProjMatrix[i];
        texelSpaceOrtho[3] += roundedShadowOffset;
        cascadeProjMatrix[i] = texelSpaceOrtho;

        this->passData.cascades[i] = cascadeProjMatrix[i] * cascadeViewMatrix[i];
        lightCullingFrustums[i] = buildCameraFrustum(this->passData.cascades[i], -lightDir);

        previousCascadeDistance = cascadeSplits[i];

        this->passData.cascadeSplits[i].x = minZ + (cascadeSplits[i] * clipRange);
      }
    }
  }
}