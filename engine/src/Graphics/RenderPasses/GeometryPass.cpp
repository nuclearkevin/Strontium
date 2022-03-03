#include "Graphics/Renderpasses/GeometryPass.h"

// Project includes.
#include "Graphics/Renderer.h"

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
    this->passData.staticGeometry = ShaderCache::getShader("geometry_pass_shader");
    this->passData.dynamicGeometry = ShaderCache::getShader("dynamic_geometry_pass");
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
    this->passData.staticDrawList.clear();
    this->passData.dynamicDrawList.clear();

    // Resize the geometry buffer.
	glm::uvec2 gBufferSize = this->passData.gBuffer.getSize();
	if (width != gBufferSize.x || height != gBufferSize.y)
	  this->passData.gBuffer.resize(width, height);

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

	// Setup the camera uniforms.
	struct CameraBlockData
	{
	  glm::mat4 viewMatrix;
      glm::mat4 projMatrix;
      glm::mat4 invViewProjMatrix;
      glm::vec4 camPosition; // w unused
      glm::vec4 nearFar; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
	} 
	  cameraBlock 
	{ 
	  this->globalBlock->sceneCam.view, 
	  this->globalBlock->sceneCam.projection, 
	  this->globalBlock->sceneCam.invViewProj,
      { this->globalBlock->sceneCam.position, 0.0 },
      { this->globalBlock->sceneCam.near,
        this->globalBlock->sceneCam.far, this->globalBlock->gamma, 0.0 }
	};
	this->passData.cameraBuffer.setData(0, sizeof(CameraBlockData), &cameraBlock);

    uint numUniqueEntities = 0;
    // Loop over static and non-skinned dynamic models + meshes to:
    // - Perform frustum culling.
    // - Compute and cache transforms. TODO: Handle skinned meshes better.
    // - Cache the entity IDs and the selection mask.
    // - Cache the uniform material data.
    // - Populate the draw lists.
    // Static meshes.
	for (auto& drawable : this->globalBlock->staticRenderQueue)
	{
	  auto& [data, materials, transform, id, drawSelectionMask] = drawable;
      this->globalBlock->drawEdge = drawSelectionMask || this->globalBlock->drawEdge;

	  for (auto& submesh : data->getSubmeshes())
	  {
        auto material = materials->getMaterial(submesh.getName());
        if (!material)
          continue;

        VertexArray* vao = submesh.hasVAO() ? submesh.getVAO() : submesh.generateVAO();
        if (!vao)
          continue;

        // Record some statistics.
        this->passData.numTrianglesSubmitted += vao->numToRender() / 3;

        auto localTransform = transform * submesh.getTransform();
        if (!boundingBoxInFrustum(this->globalBlock->camFrustum, submesh.getMinPos(), 
								  submesh.getMaxPos(), localTransform))
          continue;

        this->globalBlock->drawEdge = this->globalBlock->drawEdge || drawSelectionMask;

        // Populate the draw list.
        auto drawData = std::find_if(this->passData.staticDrawList.begin(), 
                                     this->passData.staticDrawList.end(),
                                     [vao, material](const GeomStaticDrawData& data)
        {
          return data.technique == material && data.primatives == vao;
        });

        numUniqueEntities++;
        if (drawData != this->passData.staticDrawList.end())
        {
          drawData->instancedData.emplace_back(localTransform, 
                                               glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                         id + 1.0f, 0.0f, 0.0f), 
                                               material->getPackedUniformData());
        }
        else 
        {
          this->passData.staticDrawList.emplace_back(vao, material);
          this->passData.staticDrawList.back().instancedData.emplace_back(localTransform, 
                                                                          glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                                                    id + 1.0f, 0.0f, 0.0f), 
                                                                          material->getPackedUniformData());
        }
	  }
	}

    // Dynamic meshes, skinned or rigged.
	for (auto& drawable : this->globalBlock->dynamicRenderQueue)
	{
	  auto& [data, animations, materials, transform, id, drawSelectionMask] = drawable;
      this->globalBlock->drawEdge = drawSelectionMask || this->globalBlock->drawEdge;

      if (data->hasSkins())
      {
        // Skinned animated mesh, store the static transform.
        for (auto& submesh : data->getSubmeshes())
	    {
          auto material = materials->getMaterial(submesh.getName());
          if (!material)
            continue;

          VertexArray* vao = submesh.hasVAO() ? submesh.getVAO() : submesh.generateVAO();
          if (!vao)
            continue;

          // Record some statistics.
          this->passData.numTrianglesSubmitted += vao->numToRender() / 3;

          if (!boundingBoxInFrustum(this->globalBlock->camFrustum, data->getMinPos(),
		  						    data->getMaxPos(), transform))
            continue;

          this->globalBlock->drawEdge = this->globalBlock->drawEdge || drawSelectionMask;

          // Populate the dynamic draw list.
          numUniqueEntities++;
          this->passData.dynamicDrawList.emplace_back(vao, material, animations,
                                                      PerEntityData(transform,
                                                      glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                                id + 1.0f, 0.0f, 0.0f), 
                                                      material->getPackedUniformData()));
	    }
      }
      else
      {
        // Unskinned animated mesh, store the rigged transform.
	    auto& bones = animations->getFinalUnSkinnedTransforms();
	    for (auto& submesh : data->getSubmeshes())
	    {
          auto material = materials->getMaterial(submesh.getName());
          if (!material)
            continue;

          VertexArray* vao = submesh.hasVAO() ? submesh.getVAO() : submesh.generateVAO();
          if (!vao)
            continue;

          // Record some statistics.
          this->passData.numTrianglesSubmitted += vao->numToRender() / 3;

          auto localTransform = transform * bones[submesh.getName()];
		  if (!boundingBoxInFrustum(this->globalBlock->camFrustum, submesh.getMinPos(), 
		  						    submesh.getMaxPos(), localTransform))
            continue;

          this->globalBlock->drawEdge = this->globalBlock->drawEdge || drawSelectionMask;

          // Populate the draw list.
          auto drawData = std::find_if(this->passData.staticDrawList.begin(), 
                                       this->passData.staticDrawList.end(),
                                       [vao, material](const GeomStaticDrawData& data)
          {
            return data.technique == material && data.primatives == vao;
          });
          
          numUniqueEntities++;
          if (drawData != this->passData.staticDrawList.end())
          {
            drawData->instancedData.emplace_back(localTransform, 
                                                 glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                           id + 1.0f, 0.0f, 0.0f), 
                                                 material->getPackedUniformData());
          }
          else 
          {
            this->passData.staticDrawList.emplace_back(vao, material);
            this->passData.staticDrawList.back().instancedData.emplace_back(localTransform, 
                                                                            glm::vec4(drawSelectionMask ? 1.0f : 0.0f, 
                                                                                      id + 1.0f, 0.0f, 0.0f), 
                                                                            material->getPackedUniformData());
          }
	    }
      }
	}
    
    // Upload the cached data for both static and dynamic geometry.
    if (this->passData.entityDataBuffer.size() != (sizeof(PerEntityData) * numUniqueEntities))
      this->passData.entityDataBuffer.resize(sizeof(PerEntityData) * numUniqueEntities, BufferType::Dynamic);

    uint bufferPointer = 0;
    for (auto& drawCommand : this->passData.staticDrawList)
    {
      this->passData.entityDataBuffer.setData(bufferPointer, sizeof(PerEntityData) * drawCommand.instancedData.size(), 
                                              drawCommand.instancedData.data());
      bufferPointer += sizeof(PerEntityData) * drawCommand.instancedData.size();
    }
    for (auto& drawCommand : this->passData.dynamicDrawList)
    {
      this->passData.entityDataBuffer.setData(bufferPointer, sizeof(PerEntityData), 
                                              &drawCommand.data);
      bufferPointer += sizeof(PerEntityData);
    }

	// Start the geometry pass.
    this->passData.gBuffer.beginGeoPass();

    this->passData.cameraBuffer.bindToPoint(0);
    this->passData.perDrawUniforms.bindToPoint(1);

    this->passData.entityDataBuffer.bindToPoint(0);

    uint bufferOffset = 0;
    // Static geometry pass.
    this->passData.staticGeometry->bind();
    for (auto& drawable : this->passData.staticDrawList)
    {
      // Set the index offset. 
      this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);

      drawable.technique->configureTextures();

      drawable.primatives->bind();
      RendererCommands::drawElementsInstanced(PrimativeType::Triangle, 
                                              drawable.primatives->numToRender(), 
                                              drawable.instancedData.size());
      drawable.primatives->unbind();
      
      bufferOffset += drawable.instancedData.size();

      // Record some statistics.
      this->passData.numDrawCalls++;
      this->passData.numInstances += drawable.instancedData.size();
      this->passData.numTrianglesDrawn += (drawable.instancedData.size() * drawable.primatives->numToRender()) / 3;
    }

    // Dynamic geometry pass for skinned objects.
    // TODO: Improve this with compute shader skinning. 
    // Could probably get rid of this pass all together.
    this->passData.dynamicGeometry->bind();
    this->passData.boneBuffer.bindToPoint(4);
    for (auto& drawable : this->passData.dynamicDrawList)
    {
      auto& bones = static_cast<Animator*>(drawable)->getFinalBoneTransforms();
      this->passData.boneBuffer.setData(0, bones.size() * sizeof(glm::mat4),
                                        bones.data());
      
      // Set the index offset. 
      this->passData.perDrawUniforms.setData(0, sizeof(int), &bufferOffset);

      drawable.technique->configureTextures();
      
      drawable.primatives->bind();
      RendererCommands::drawElementsInstanced(PrimativeType::Triangle, 
                                              drawable.primatives->numToRender(), 
                                              drawable.instanceCount);
      drawable.primatives->unbind();

      bufferOffset += drawable.instanceCount;

      // Record some statistics.
      this->passData.numDrawCalls++;
      this->passData.numInstances += drawable.instanceCount;
      this->passData.numTrianglesDrawn += (drawable.instanceCount * drawable.primatives->numToRender()) / 3;
    }
    this->passData.dynamicGeometry->unbind();

    this->passData.gBuffer.endGeoPass();
  }

  void 
  GeometryPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  GeometryPass::onShutdown()
  {

  }
}