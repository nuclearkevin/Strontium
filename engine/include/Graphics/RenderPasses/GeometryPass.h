#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Model.h"
#include "Graphics/Animations.h"
#include "Graphics/Material.h"
#include "Graphics/GeometryBuffer.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  namespace Renderer3D
  {
  	struct GlobalRendererData;
  }

  struct PerEntityData
  {
	glm::mat4 transform;
	glm::vec4 idMask;
	MaterialBlockData materialData;

	PerEntityData(const glm::mat4 &transform, const glm::vec4 &idMask,
				  const MaterialBlockData& materialData)
	  : transform(transform)
	  , idMask(idMask)
	  , materialData(materialData)
	{ }
  };

  struct GeomMeshData
  {
	DrawArraysIndirectCommand drawData;
	Material* technique;
	bool draw;

	std::vector<PerEntityData> instanceData;

	GeomMeshData(uint count, uint instanceCount, uint first, uint baseInstance, Material* technique)
	  : drawData(count, instanceCount, first, baseInstance)
	  , technique(technique)
	  , draw(false)
	{ }
  };

  struct GeomDynamicDrawData
  {
	Material* technique;
	Animator* animations;

	uint numToRender;
	uint globalBufferOffset;

	PerEntityData data;

	uint instanceCount;

	GeomDynamicDrawData(uint globalBufferOffset, Material* technique,
						uint numToRender, Animator* animations, const PerEntityData &data)
	  : globalBufferOffset(globalBufferOffset)
      , technique(technique)
	  , numToRender(numToRender)
	  , animations(animations)
	  , data(data)
	  , instanceCount(1)
	{ }
  };
}

namespace Strontium
{
  struct GeometryPassDataBlock
  {
	// Required buffers and lists to draw stuff.
	GeometryBuffer gBuffer;
	FrameBuffer idMaskBuffer;

	Shader* staticGeometryPass;
	Shader* dynamicGeometryPass;
	Shader* staticEditorPass;
	Shader* dynamicEditorPass;

	UniformBuffer perDrawUniforms;

	ShaderStorageBuffer entityDataBuffer;
	ShaderStorageBuffer boneBuffer;

	uint numUniqueEntities;
	robin_hood::unordered_flat_map<Model*, uint> modelMap;
	std::vector<GeomMeshData> staticGeometry;
	std::vector<GeomDynamicDrawData> dynamicDrawList;
	bool drawingIDs;
	bool drawingMask;

	// Some statistics to display.
	float frameTime;
	uint numInstances;
	uint numDrawCalls;
	uint numTrianglesSubmitted;
	uint numTrianglesDrawn;
	
	GeometryPassDataBlock()
	  : gBuffer(1600u, 900u)
	  , idMaskBuffer(1600u, 900u)
	  , staticGeometryPass(nullptr)
	  , dynamicGeometryPass(nullptr)
	  , staticEditorPass(nullptr)
	  , dynamicEditorPass(nullptr)
	  , perDrawUniforms(sizeof(int), BufferType::Dynamic)
	  , entityDataBuffer(0, BufferType::Dynamic)
	  , boneBuffer(MAX_BONES_PER_MODEL * sizeof(glm::mat4), BufferType::Dynamic)
	  , numUniqueEntities(0u)
	  , drawingIDs(false)
	  , drawingMask(false)
	  , frameTime(0.0f)
	  , numInstances(0u)
	  , numDrawCalls(0u)
	  , numTrianglesSubmitted(0u)
	  , numTrianglesDrawn(0u)
	{ }
  };

  class GeometryPass final : public RenderPass
  {
  public:
	GeometryPass(Renderer3D::GlobalRendererData* globalRendererData);
	~GeometryPass() override;

	void onInit() override;
	void updatePassData() override;
	RendererDataHandle requestRendererData() override;
	void deleteRendererData(RendererDataHandle& handle) override;
	void onRendererBegin(uint width, uint height) override;
	void onRender() override;
	void onRendererEnd(FrameBuffer& frontBuffer) override;
	void onShutdown() override;

	void submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                float id = -1.0f, bool drawSelectionMask = false);
    void submit(Model* data, Animator* animation, ModelMaterial& materials,
                const glm::mat4 &model, float id = -1.0f,
                bool drawSelectionMask = false);
  private:
	GeometryPassDataBlock passData;

	AsynchTimer timer;
  };
}