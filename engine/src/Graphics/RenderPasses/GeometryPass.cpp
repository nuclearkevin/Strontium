#include "Graphics/Renderpasses/GeometryPass.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RendererCommands.h"

namespace Strontium
{
  GeometryPass::GeometryPass(Renderer3D::GlobalRendererData* globalRendererData)
	: RenderPass(&this->passData, globalRendererData, { nullptr })
    , timer(5)
  { }

  GeometryPass::~GeometryPass()
  { }
  
  void 
  GeometryPass::onInit()
  {
    this->passData.staticGeometryPass = ShaderCache::getShader("static_geometry_pass");
    this->passData.dynamicGeometryPass = ShaderCache::getShader("dynamic_geometry_pass");
    this->passData.staticEditorPass = ShaderCache::getShader("static_editor_pass");
    this->passData.dynamicEditorPass = ShaderCache::getShader("dynamic_editor_pass");

    // Init the editor-specific drawable parameters.
    auto cSpec = Texture2D::getFloatColourParams();
    auto dSpec = Texture2D::getDefaultDepthParams();
    
    cSpec.internal = TextureInternalFormats::RG16f;
    cSpec.format = TextureFormats::RG;
    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    dSpec.sWrap = TextureWrapParams::ClampEdges;
    dSpec.tWrap = TextureWrapParams::ClampEdges;

    auto cAttachment = FBOAttachment(FBOTargetParam::Colour0, FBOTextureParam::Texture2D,
                                     cSpec.internal, cSpec.format, cSpec.dataType);
    auto dAttachment = FBOAttachment(FBOTargetParam::Depth, FBOTextureParam::Texture2D,
                                     dSpec.internal, dSpec.format, dSpec.dataType);
    // The entity ID and mask texture.
    this->passData.idMaskBuffer.attach(cSpec, cAttachment);
    this->passData.idMaskBuffer.attach(dSpec, dAttachment);
    this->passData.idMaskBuffer.setDrawBuffers();
    this->passData.idMaskBuffer.setClearColour(glm::vec4(0.0f));
  }

  void 
  GeometryPass::updatePassData()
  { }

  RendererDataHandle 
  GeometryPass::requestRendererData()
  {
    return -1;
  }

  void 
  GeometryPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  GeometryPass::onRendererBegin(uint width, uint height)
  {
    // Clear the lists and staging for the next frame.
    this->passData.modelMap.clear();
    this->passData.staticGeometry.clear();
    this->passData.dynamicDrawList.clear();

    // Resize the geometry buffer.
	glm::uvec2 gBufferSize = this->passData.gBuffer.getSize();
	if (width != gBufferSize.x || height != gBufferSize.y)
    {
	  this->passData.gBuffer.resize(width, height);
      this->passData.idMaskBuffer.resize(width, height);
    }

    this->passData.idMaskBuffer.clear();

    this->passData.numUniqueEntities = 0u;
    this->passData.drawingIDs = false;
    this->passData.drawingMask = false;

    // Clear the statistics.
    this->passData.numDrawCalls = 0u;
    this->passData.numInstances = 0u;
    this->passData.numTrianglesSubmitted = 0u;
    this->passData.numTrianglesDrawn = 0u;
  }

  void 
  GeometryPass::onRender()
  {
    // Time the entire scope.
    ScopedTimer<AsynchTimer> profiler(this->timer);

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    
    // Upload the cached data for both static and dynamic geometry.
    if (this->passData.entityDataBuffer.size() < (sizeof(PerEntityData) * this->passData.numUniqueEntities))
      this->passData.entityDataBuffer.resize(sizeof(PerEntityData) * this->passData.numUniqueEntities, BufferType::Static);

    uint bufferOffset = 0;
    for (auto& geometry : this->passData.staticGeometry)
    {
      this->passData.entityDataBuffer.setData(bufferOffset, sizeof(PerEntityData) * geometry.drawData.instanceCount,
                                              geometry.instanceData.data());
      bufferOffset += sizeof(PerEntityData) * geometry.drawData.instanceCount;
    }
    for (auto& drawCommand : this->passData.dynamicDrawList)
    {
      this->passData.entityDataBuffer.setData(bufferOffset, sizeof(PerEntityData),
                                              &drawCommand.data);
      bufferOffset += sizeof(PerEntityData);
    }

	// Start the geometry pass.
    this->passData.gBuffer.beginGeoPass();

    rendererData->cameraBuffer.bindToPoint(0);
    this->passData.perDrawUniforms.bindToPoint(1);

    this->passData.entityDataBuffer.bindToPoint(2);

    bufferOffset = 0;
    rendererData->blankVAO.bind();
    rendererData->vertexCache.bindToPoint(0);
    rendererData->indexCache.bindToPoint(1);
    // Static geometry pass.
    this->passData.staticGeometryPass->bind();
    for (auto& geometry : this->passData.staticGeometry)
    {
      // Set the index offset. 
      this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);

      geometry.technique->configureTextures();

      RendererCommands::drawArraysInstanced(PrimativeType::Triangle, geometry.drawData.first, geometry.drawData.count,
                                            geometry.drawData.instanceCount);
      
      bufferOffset += geometry.drawData.instanceCount;

      // Record some statistics.
      this->passData.numDrawCalls++;
      this->passData.numInstances += geometry.drawData.instanceCount;
      this->passData.numTrianglesDrawn += (geometry.drawData.instanceCount * geometry.drawData.count) / 3;
    }

    // Dynamic geometry pass for skinned objects.
    // TODO: Improve this with compute shader skinning. 
    // Could probably get rid of this pass all together.
    this->passData.dynamicGeometryPass->bind();
    this->passData.boneBuffer.bindToPoint(3);
    for (auto& drawable : this->passData.dynamicDrawList)
    {
      auto& bones = drawable.animations->getFinalBoneTransforms();
      this->passData.boneBuffer.setData(0, bones.size() * sizeof(glm::mat4),
                                        bones.data());
      
      // Set the index offset. 
      this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);

      drawable.technique->configureTextures();
      
      RendererCommands::drawArraysInstanced(PrimativeType::Triangle, drawable.globalBufferOffset, drawable.numToRender,
                                            drawable.instanceCount);

      bufferOffset += drawable.instanceCount;

      // Record some statistics.
      this->passData.numDrawCalls++;
      this->passData.numInstances += drawable.instanceCount;
      this->passData.numTrianglesDrawn += (drawable.instanceCount * drawable.numToRender) / 3;
    }
    this->passData.dynamicGeometryPass->unbind();

    this->passData.gBuffer.endGeoPass();

    // Start the editor pass.
    if (this->passData.drawingIDs || this->passData.drawingMask)
    {
      this->passData.idMaskBuffer.setViewport();
      this->passData.idMaskBuffer.bind();
      
      rendererData->cameraBuffer.bindToPoint(0);
      this->passData.perDrawUniforms.bindToPoint(1);
      
      this->passData.entityDataBuffer.bindToPoint(2);
      
      bufferOffset = 0;
      rendererData->blankVAO.bind();
      // Static geometry pass.
      this->passData.staticEditorPass->bind();
      for (auto& geometry : this->passData.staticGeometry)
      {
        // Set the index offset. 
        if (geometry.draw)
        {
          this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);
      
          geometry.technique->configureTextures();
      
          RendererCommands::drawArraysInstanced(PrimativeType::Triangle, geometry.drawData.first, geometry.drawData.count,
                                                geometry.drawData.instanceCount);
          // Record some statistics.
          this->passData.numDrawCalls++;
          this->passData.numInstances += geometry.drawData.instanceCount;
          this->passData.numTrianglesDrawn += (geometry.drawData.instanceCount * geometry.drawData.count) / 3;
        }
        
        bufferOffset += geometry.drawData.instanceCount;
      }
      
      // Dynamic geometry pass for skinned objects.
      // TODO: Improve this with compute shader skinning. 
      // Could probably get rid of this pass all together.
      this->passData.dynamicEditorPass->bind();
      this->passData.boneBuffer.bindToPoint(3);
      for (auto& drawable : this->passData.dynamicDrawList)
      {
        auto& bones = drawable.animations->getFinalBoneTransforms();
        this->passData.boneBuffer.setData(0, bones.size() * sizeof(glm::mat4),
                                          bones.data());
        
        // Set the index offset. 
        this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);
      
        drawable.technique->configureTextures();
        
        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, drawable.globalBufferOffset, drawable.numToRender,
                                              drawable.instanceCount);
      
        bufferOffset += drawable.instanceCount;
      
        // Record some statistics.
        this->passData.numDrawCalls++;
        this->passData.numInstances += drawable.instanceCount;
        this->passData.numTrianglesDrawn += (drawable.instanceCount * drawable.numToRender) / 3;
      }
      this->passData.dynamicGeometryPass->unbind();
      
      this->passData.idMaskBuffer.unbind();
    }
  }

  void 
  GeometryPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  GeometryPass::onShutdown()
  { }

  // TODO: Fix out of bounds error on submission.
  void 
  GeometryPass::submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                       float id, bool drawSelectionMask)
  {
    if (!data->isDrawable())
    {
      if (!data->init())
        return;
    }

    auto& cameraFrustum = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock)->camFrustum;

    uint numSubmeshes = data->getSubmeshes().size();
    auto modelLoc = this->passData.modelMap.find(data);
    if (modelLoc != this->passData.modelMap.end())
    {
      uint meshStart = modelLoc->second;
      for (uint i = 0u; i < numSubmeshes; ++i)
      {
        auto& submesh = data->getSubmeshes()[i];
        auto material = materials.getMaterial(submesh.getName());
        if (!material)
          continue;
        /*
        if (!submesh.isDrawable())
          continue;
        */
        const auto localTransform = model * submesh.getTransform();
        if (!boundingBoxInFrustum(cameraFrustum, submesh.getMinPos(), submesh.getMaxPos(), localTransform))
          continue;

        // Store the submesh draw data.
        this->passData.staticGeometry[meshStart + i].instanceData.emplace_back(localTransform,
                                                                               glm::vec4(drawSelectionMask ? 1.0f : 0.0f, id + 1.0f, 0.0f, 0.0f),
                                                                               material->getPackedUniformData());
        this->passData.staticGeometry[meshStart + i].drawData.instanceCount++;

        this->passData.numUniqueEntities++;
      }
    }
    else
    {
      uint meshStart = this->passData.staticGeometry.size();
      
      // Allocate space for the draw data.
      this->passData.staticGeometry.reserve(this->passData.staticGeometry.size() + numSubmeshes);
      this->passData.modelMap.emplace(data, meshStart);
      for (uint i = 0u; i < numSubmeshes; ++i)
      {
        auto& submesh = data->getSubmeshes()[i];
        auto material = materials.getMaterial(submesh.getName());
        if (!material)
          continue;
        /*
        if (!submesh.isDrawable())
          continue;
        */
        // Compute the scene AABB.
        const auto& localTransform = model * submesh.getTransform();

        // Store the new submesh draw data.
        this->passData.staticGeometry.emplace_back(submesh.numToRender(), 1u, submesh.getGlobalLocation(), 0u, material);
        if (boundingBoxInFrustum(cameraFrustum, submesh.getMinPos(), submesh.getMaxPos(), localTransform))
        {
          this->passData.staticGeometry.back().draw = true;
          this->passData.numUniqueEntities++;
          this->passData.staticGeometry.back().instanceData.emplace_back(localTransform,
                                                                         glm::vec4(drawSelectionMask ? 1.0f : 0.0f, id + 1.0f, 0.0f, 0.0f),
                                                                         material->getPackedUniformData());
        }
      }
    }

    this->passData.drawingMask = this->passData.drawingMask || drawSelectionMask;
    this->passData.drawingIDs = this->passData.drawingIDs || (id >= 0.0f);
  }

  // TODO: Fix out of bounds error on submission.
  void 
  GeometryPass::submit(Model* data, Animator* animation, ModelMaterial &materials,
                       const glm::mat4 &model, float id, bool drawSelectionMask)
  {
    if (!data->isDrawable())
    {
      if (!data->init())
        return;
    }

    auto& cameraFrustum = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock)->camFrustum;

    this->passData.drawingMask = this->passData.drawingMask || drawSelectionMask;
    this->passData.drawingIDs = this->passData.drawingIDs || (id >= 0.0f);

    if (data->hasSkins())
    {
      // Skinned animated mesh, store the static transform.
      for (auto& submesh : data->getSubmeshes())
	  {
        auto material = materials.getMaterial(submesh.getName());
        if (!material)
          continue;

        if (!submesh.isDrawable())
          continue;

        // Record some statistics.
        this->passData.numTrianglesSubmitted += submesh.numToRender() / 3;

        if (!boundingBoxInFrustum(cameraFrustum, data->getMinPos(),
	 						      data->getMaxPos(), model))
          continue;

        // Populate the dynamic draw list.
        this->passData.numUniqueEntities++;
        this->passData.dynamicDrawList.emplace_back(submesh.getGlobalLocation(), material, submesh.numToRender(), animation,
                                                    PerEntityData(model,
                                                    glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                              id + 1.0f, 0.0f, 0.0f), 
                                                    material->getPackedUniformData()));
	  }
    }
    else
    {
      // Unskinned animated mesh, store the rigged transform.
	  auto& bones = animation->getFinalUnSkinnedTransforms();
	  uint numSubmeshes = data->getSubmeshes().size();
      auto modelLoc = this->passData.modelMap.find(data);
      if (modelLoc != this->passData.modelMap.end())
      {
        uint meshStart = modelLoc->second;
        for (uint i = 0u; i < numSubmeshes; ++i)
        {
          auto& submesh = data->getSubmeshes()[i];
          auto material = materials.getMaterial(submesh.getName());
          if (!material)
            continue;
          /*
          if (!submesh.isDrawable())
            continue;
          */
          const auto localTransform = model * submesh.getTransform();
          if (!boundingBoxInFrustum(cameraFrustum, submesh.getMinPos(), submesh.getMaxPos(), localTransform))
            continue;
      
          // Store the submesh draw data.
          this->passData.staticGeometry[meshStart + i].instanceData.emplace_back(localTransform,
                                                                                 glm::vec4(drawSelectionMask ? 1.0f : 0.0f, id + 1.0f, 0.0f, 0.0f),
                                                                                 material->getPackedUniformData());
          this->passData.staticGeometry[meshStart + i].drawData.instanceCount++;
      
          this->passData.numUniqueEntities++;
        }
      }
      else
      {
        uint meshStart = this->passData.staticGeometry.size();
        
        // Allocate space for the draw data.
        this->passData.staticGeometry.reserve(this->passData.staticGeometry.size() + numSubmeshes);
        this->passData.modelMap.emplace(data, meshStart);
        for (uint i = 0u; i < numSubmeshes; ++i)
        {
          auto& submesh = data->getSubmeshes()[i];
          auto material = materials.getMaterial(submesh.getName());
          if (!material)
            continue;
          /*
          if (!submesh.isDrawable())
            continue;
          */
          // Compute the scene AABB.
          const auto localTransform = model * bones[submesh.getName()];
      
          // Store the new submesh draw data.
          this->passData.staticGeometry.emplace_back(submesh.numToRender(), 1u, submesh.getGlobalLocation(), 0u, material);
          if (boundingBoxInFrustum(cameraFrustum, submesh.getMinPos(), submesh.getMaxPos(), localTransform))
          {
            this->passData.staticGeometry.back().draw = true;
            this->passData.numUniqueEntities++;
            this->passData.staticGeometry.back().instanceData.emplace_back(localTransform,
                                                                           glm::vec4(drawSelectionMask ? 1.0f : 0.0f, id + 1.0f, 0.0f, 0.0f),
                                                                           material->getPackedUniformData());
          }
        }
      }  
	}
  }
}