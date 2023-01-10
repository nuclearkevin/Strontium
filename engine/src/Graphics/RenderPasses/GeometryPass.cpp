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
    this->passData.staticGeometry = ShaderCache::getShader("static_geometry_pass");
    this->passData.dynamicGeometry = ShaderCache::getShader("dynamic_geometry_pass");
    this->passData.staticEditor = ShaderCache::getShader("static_editor_pass");
    this->passData.dynamicEditor = ShaderCache::getShader("dynamic_editor_pass");

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
    for (auto& [drawable, instancedData] : this->passData.staticInstanceMap)
      instancedData.clear();

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
    for (auto& [vaoMat, instancedData] : this->passData.staticInstanceMap)
    {
      this->passData.entityDataBuffer.setData(bufferOffset, sizeof(PerEntityData) * instancedData.size(),
                                              instancedData.data());
      bufferOffset += sizeof(PerEntityData) * instancedData.size();
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
    this->passData.staticGeometry->bind();
    for (auto& [drawable, instancedData] : this->passData.staticInstanceMap)
    {
      if (instancedData.size() == 0u)
        continue;
        
      // Set the index offset. 
      this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);

      drawable.technique->configureTextures();

      RendererCommands::drawArraysInstanced(PrimativeType::Triangle, drawable.globalBufferOffset, drawable.numToRender,
                                            instancedData.size());
      
      bufferOffset += instancedData.size();

      // Record some statistics.
      this->passData.numDrawCalls++;
      this->passData.numInstances += instancedData.size();
      this->passData.numTrianglesDrawn += (instancedData.size() * drawable.numToRender) / 3;
    }

    // Dynamic geometry pass for skinned objects.
    // TODO: Improve this with compute shader skinning. 
    // Could probably get rid of this pass all together.
    this->passData.dynamicGeometry->bind();
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
    this->passData.dynamicGeometry->unbind();

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
      this->passData.staticEditor->bind();
      for (auto& [drawable, instancedData] : this->passData.staticInstanceMap)
      {
        // Set the index offset. 
        this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);
      
        drawable.technique->configureTextures();
      
        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, drawable.globalBufferOffset, drawable.numToRender,
                                              instancedData.size());
        
        bufferOffset += instancedData.size();
      
        // Record some statistics.
        this->passData.numDrawCalls++;
        this->passData.numInstances += instancedData.size();
        this->passData.numTrianglesDrawn += (instancedData.size() * drawable.numToRender) / 3;
      }
      
      // Dynamic geometry pass for skinned objects.
      // TODO: Improve this with compute shader skinning. 
      // Could probably get rid of this pass all together.
      this->passData.dynamicEditor->bind();
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
      this->passData.dynamicGeometry->unbind();
      
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

  void 
  GeometryPass::submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                       float id, bool drawSelectionMask)
  {
    if (!data->isDrawable())
    {
      if (!data->init())
        return;
    }

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);

    this->passData.drawingMask = this->passData.drawingMask || drawSelectionMask;
    this->passData.drawingIDs = this->passData.drawingIDs || (id >= 0.0f);

	for (auto& submesh : data->getSubmeshes())
	{
      auto material = materials.getMaterial(submesh.getName());
      if (!material)
        continue;

      if (!submesh.isDrawable())
        continue;

      // Record some statistics.
      this->passData.numTrianglesSubmitted += submesh.numToRender() / 3;

      auto localTransform = model * submesh.getTransform();
      if (!boundingBoxInFrustum(rendererData->camFrustum, submesh.getMinPos(),
						  submesh.getMaxPos(), localTransform))
        continue;

      // Populate the draw list.
      auto drawData = this->passData.staticInstanceMap.find(GeomStaticDrawData(submesh.getGlobalLocation(), material, 
                                                                               submesh.numToRender()));

      this->passData.numUniqueEntities++;
      if (drawData != this->passData.staticInstanceMap.end())
      {
        drawData->second.emplace_back(localTransform, 
                                      glm::vec4(drawSelectionMask ? 1.0f : 0.0f, id + 1.0f, 0.0f, 0.0f), 
                                      material->getPackedUniformData());
      }
      else 
      {
        auto& item = this->passData.staticInstanceMap.emplace(GeomStaticDrawData(submesh.getGlobalLocation(), material, 
                                                                                 submesh.numToRender()), 
                                                                                 std::vector<PerEntityData>());
        item.first->second.emplace_back(localTransform, glm::vec4(drawSelectionMask ? 1.0f : 0.0f, id + 1.0f, 0.0f, 0.0f), 
                                        material->getPackedUniformData());
      }
	}
  }

  void 
  GeometryPass::submit(Model* data, Animator* animation, ModelMaterial &materials,
                       const glm::mat4 &model, float id, bool drawSelectionMask)
  {
    if (!data->isDrawable())
    {
      if (!data->init())
        return;
    }

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);

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

        if (!boundingBoxInFrustum(rendererData->camFrustum, data->getMinPos(),
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
	  for (auto& submesh : data->getSubmeshes())
	  {
        auto material = materials.getMaterial(submesh.getName());
        if (!material)
          continue;

        if (!submesh.isDrawable())
          continue;

        // Record some statistics.
        this->passData.numTrianglesSubmitted += submesh.numToRender() / 3;

        auto localTransform = model * bones[submesh.getName()];
	    if (!boundingBoxInFrustum(rendererData->camFrustum, submesh.getMinPos(),
	 						      submesh.getMaxPos(), localTransform))
          continue;

        // Populate the draw list.
        auto drawData = this->passData.staticInstanceMap.find(GeomStaticDrawData(submesh.getGlobalLocation(), material, 
                                                                                 submesh.numToRender()));
        
        this->passData.numUniqueEntities++;
        if (drawData != this->passData.staticInstanceMap.end())
        {
          this->passData.staticInstanceMap.at(drawData->first)
                                          .emplace_back(localTransform, glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                        id + 1.0f, 0.0f, 0.0f), material->getPackedUniformData());
        }
        else 
        {
          this->passData.staticInstanceMap.emplace(GeomStaticDrawData(submesh.getGlobalLocation(), material, 
                                                                      submesh.numToRender()),
                                                   std::vector<PerEntityData>());
          this->passData.staticInstanceMap.at(GeomStaticDrawData(submesh.getGlobalLocation(), material, submesh.numToRender()))
                                          .emplace_back(localTransform, glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                        id + 1.0f, 0.0f, 0.0f), material->getPackedUniformData());
        }
      }  
	}
  }
}