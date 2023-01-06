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

    this->passData.shadowBuffer.resize(NUM_CASCADES * this->passData.shadowMapRes, this->passData.shadowMapRes);
    this->passData.shadowBuffer.attach(dSpec, depthAttachment);
  }
  
  void 
  ShadowPass::updatePassData()
  {
    if (this->passData.shadowMapRes != 
        static_cast<uint>(this->passData.shadowBuffer.getSize().y))
      this->passData.shadowBuffer.resize(NUM_CASCADES * this->passData.shadowMapRes, this->passData.shadowMapRes);
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
    this->passData.shadowBuffer.clear();

    this->passData.castShadows = false;
    this->passData.hasCascades = false;

    this->passData.numUniqueEntities = 0u;
    this->passData.staticInstanceMap.clear();
    this->passData.dynamicDrawList.clear();
    this->passData.minPos = glm::vec3(std::numeric_limits<float>::max());
    this->passData.maxPos = glm::vec3(std::numeric_limits<float>::min());

	// Clear the statistics.
    this->passData.numDrawCalls = 0u;
    this->passData.numInstances = 0u;
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
    if (this->passData.transformBuffer.size() < (sizeof(glm::mat4) * this->passData.numUniqueEntities))
      this->passData.transformBuffer.resize(sizeof(glm::mat4) * this->passData.numUniqueEntities, BufferType::Dynamic);
    
    uint bufferPointer = 0;
    for (auto& [drawable, instancedTransforms] : this->passData.staticInstanceMap)
    {
      this->passData.transformBuffer.setData(bufferPointer, sizeof(glm::mat4) * instancedTransforms.size(),
                                             instancedTransforms.data());
      bufferPointer += sizeof(glm::mat4) * instancedTransforms.size();
    }
    for (auto& drawCommand : this->passData.dynamicDrawList)
    {
      this->passData.transformBuffer.setData(bufferPointer, sizeof(glm::mat4),
                                             &drawCommand.transform);
      bufferPointer += sizeof(glm::mat4);
    }

    this->passData.lightSpaceBuffer.bindToPoint(0);
    this->passData.perDrawUniforms.bindToPoint(1);
    this->passData.transformBuffer.bindToPoint(2);

    // Run the Strontium render pipeline for each shadow cascade.
    static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock)->blankVAO.bind();
    static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock)->vertexCache.bindToPoint(0);
    static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock)->indexCache.bindToPoint(1);
    //RendererCommands::cullType(FaceType::Front);
    RendererCommands::disable(RendererFunction::CullFaces);
    this->passData.shadowBuffer.bind();
    for (uint i = 0; i < NUM_CASCADES; i++)
    {
      this->passData.lightSpaceBuffer.setData(0, sizeof(glm::mat4), glm::value_ptr(this->passData.cascades[i]));

      // Begin the actual shadow pass for this cascade.
      this->passData.shadowBuffer.setViewport(this->passData.shadowBuffer.getSize().y, 
                                              this->passData.shadowBuffer.getSize().y, 
                                              i * this->passData.shadowBuffer.getSize().y, 
                                              0u);
      
      bufferPointer = 0;
      // Static shadow pass.
      this->passData.staticShadow->bind();
      for (auto& [drawable, instancedTransforms] : this->passData.staticInstanceMap)
      {
        // Set the index offset. 
        this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferPointer);
      
        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, drawable.globalBufferOffset, 
                                              drawable.numToRender,
                                              instancedTransforms.size());
        
        bufferPointer += instancedTransforms.size();
      
        // Record some statistics.
        this->passData.numDrawCalls++;
        this->passData.numInstances += instancedTransforms.size();
        this->passData.numTrianglesDrawn += (instancedTransforms.size() * drawable.numToRender) / 3;
      }
      
      // Dynamic geometry pass for skinned objects.
      // TODO: Improve this with compute shader skinning. 
      // Could probably get rid of this pass all together.
      this->passData.dynamicShadow->bind();
      this->passData.boneBuffer.bindToPoint(3);
      for (auto& drawable : this->passData.dynamicDrawList)
      {
        auto& bones = drawable.animations->getFinalBoneTransforms();
        this->passData.boneBuffer.setData(0, bones.size() * sizeof(glm::mat4),
                                          bones.data());
        
        // Set the index offset. 
        this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferPointer);

        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, drawable.globalBufferOffset, 
                                              drawable.numToRender,
                                              drawable.instanceCount);
      
        bufferPointer += drawable.instanceCount;
      
        // Record some statistics.
        this->passData.numDrawCalls++;
        this->passData.numInstances += drawable.instanceCount;
        this->passData.numTrianglesDrawn += (drawable.instanceCount * drawable.numToRender) / 3;
      }
    }
    //RendererCommands::cullType(FaceType::Back);
    this->passData.dynamicShadow->unbind();
    this->passData.shadowBuffer.unbind();
    RendererCommands::enable(RendererFunction::CullFaces);
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
      if (!submesh.isDrawable())
      {
        if (!submesh.init())
          continue;
      }
      
      // Compute the scene AABB.
      auto localTransform = model * submesh.getTransform();
      this->passData.minPos = glm::min(this->passData.minPos, glm::vec3(localTransform * glm::vec4(submesh.getMinPos(), 1.0f)));
      this->passData.maxPos = glm::max(this->passData.maxPos, glm::vec3(localTransform * glm::vec4(submesh.getMaxPos(), 1.0f)));
    
      // Populate the draw list.
      auto drawData = this->passData.staticInstanceMap.find(ShadowStaticDrawData(submesh.getGlobalLocation(),
                                                                                 submesh.numToRender()));
    
      this->passData.numUniqueEntities++;
      if (drawData != this->passData.staticInstanceMap.end())
      {   
        // Cache the per-entity data.
        drawData->second.emplace_back(localTransform);
      }
      else 
      {
        auto& item = this->passData.staticInstanceMap.emplace(ShadowStaticDrawData(submesh.getGlobalLocation(),
                                                                                   submesh.numToRender()), std::vector<glm::mat4>());
        item.first->second.emplace_back(localTransform);
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
        if (!submesh.isDrawable())
        {
          if (!submesh.init())
            continue;
        }
        
        this->passData.minPos = glm::min(this->passData.minPos, glm::vec3(model * glm::vec4(submesh.getMinPos(), 1.0f)));
        this->passData.maxPos = glm::max(this->passData.maxPos, glm::vec3(model * glm::vec4(submesh.getMaxPos(), 1.0f)));
    
        // Populate the dynamic draw list.
        this->passData.numUniqueEntities++;
        this->passData.dynamicDrawList.emplace_back(submesh.getGlobalLocation(), submesh.numToRender(), animation, model);
	  }
    }
    else
    {
      // Unskinned animated mesh, store the rigged transform.
	  auto& bones = animation->getFinalUnSkinnedTransforms();
	  for (auto& submesh : data->getSubmeshes())
	  {
        if (!submesh.isDrawable())
        {
          if (!submesh.init())
            continue;
        }
    
        auto localTransform = model * bones[submesh.getName()];
        this->passData.minPos = glm::min(this->passData.minPos, glm::vec3(localTransform * glm::vec4(submesh.getMinPos(), 1.0f)));
        this->passData.maxPos = glm::max(this->passData.maxPos, glm::vec3(localTransform * glm::vec4(submesh.getMaxPos(), 1.0f)));
    
        // Populate the draw list.
        auto drawData = this->passData.staticInstanceMap.find(ShadowStaticDrawData(submesh.getGlobalLocation(),
                                                                                   submesh.numToRender()));
        
        this->passData.numUniqueEntities++;
        if (drawData != this->passData.staticInstanceMap.end())
        {   
          // Cache the per-entity data.
          drawData->second.emplace_back(localTransform);
        }
        else 
        {
          auto& item = this->passData.staticInstanceMap.emplace(ShadowStaticDrawData(submesh.getGlobalLocation(),
                                                                                     submesh.numToRender()), std::vector<glm::mat4>());
          item.first->second.emplace_back(localTransform);
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
    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);

    //------------------------------------------------------------------------
    // Directional light shadow cascade calculations:
    //------------------------------------------------------------------------
    // https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
    //------------------------------------------------------------------------
    // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps
    //------------------------------------------------------------------------
    const float near = rendererData->sceneCam.near;
    const float far = rendererData->sceneCam.far;

    glm::mat4 camInvVP = rendererData->sceneCam.invViewProj;

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
        shadowOrigin = 0.5f * shadowOrigin * this->passData.shadowBuffer.getSize().y;

        glm::vec4 roundedShadowOrigin = glm::round(shadowOrigin);
        glm::vec4 roundedShadowOffset = roundedShadowOrigin - shadowOrigin;
        roundedShadowOffset = 2.0f * roundedShadowOffset / this->passData.shadowBuffer.getSize().y;
        roundedShadowOffset.z = 0.0f;
        roundedShadowOffset.w = 0.0f;

        glm::mat4 texelSpaceOrtho = cascadeProjMatrix[i];
        texelSpaceOrtho[3] += roundedShadowOffset;
        cascadeProjMatrix[i] = texelSpaceOrtho;

        this->passData.cascades[i] = cascadeProjMatrix[i] * cascadeViewMatrix[i];
        this->passData.lightCullingFrustums[i] = buildCameraFrustum(this->passData.cascades[i], -lightDir);

        previousCascadeDistance = cascadeSplits[i];

        this->passData.cascadeSplits[i].x = minZ + (cascadeSplits[i] * clipRange);
      }
    }
  }
}