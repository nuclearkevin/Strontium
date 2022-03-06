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
  struct PerEntityData
  {
	glm::mat4 transform;
	glm::vec4 idMask;
	MaterialBlockData materialData;

	PerEntityData(const glm::mat4 &transform, const glm::vec4 &idMask,
				  const MaterialBlockData&materialData)
	  : transform(transform)
	  , idMask(idMask)
	  , materialData(materialData)
	{ }
  };

  struct GeomStaticDrawData
  {
	VertexArray* primatives;
	Material* technique;

	std::vector<PerEntityData> instancedData;

	GeomStaticDrawData(VertexArray* primatives, Material* technique)
	: primatives(primatives)
    , technique(technique)
	{ }

	operator VertexArray*() { return this->primatives; }
	operator Material*() { return this->technique; }
  };

  struct GeomDynamicDrawData
  {
	VertexArray* primatives;
	Material* technique;
	Animator* animations;

	PerEntityData data;

	uint instanceCount;

	GeomDynamicDrawData(VertexArray* primatives, Material* technique, Animator* animations,
					    const PerEntityData &data)
	  : primatives(primatives)
      , technique(technique)
	  , animations(animations)
	  , data(data)
	  , instanceCount(1)
	{ }

	operator VertexArray*() { return this->primatives; }
	operator Material*() { return this->technique; }
	operator Animator* () { return this->animations; }
  };

  struct GeometryPassDataBlock
  {
	// Required buffers and lists to draw stuff.
	GeometryBuffer gBuffer;

	Shader* staticGeometry;
	Shader* dynamicGeometry;

	UniformBuffer cameraBuffer;
	UniformBuffer perDrawUniforms;

	ShaderStorageBuffer entityDataBuffer;
	ShaderStorageBuffer boneBuffer;

	uint numUniqueEntities;
	std::vector<GeomStaticDrawData> staticDrawList;
	std::vector<GeomDynamicDrawData> dynamicDrawList;

	// Some statistics to display.
	float frameTime;
	uint numInstances;
	uint numDrawCalls;
	uint numTrianglesSubmitted;
	uint numTrianglesDrawn;
	
	GeometryPassDataBlock()
	  : gBuffer(RuntimeType::Editor, 1600, 900)
	  , staticGeometry(nullptr)
	  , dynamicGeometry(nullptr)
      , cameraBuffer(3 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), BufferType::Dynamic)
	  , perDrawUniforms(sizeof(int), BufferType::Dynamic)
	  , entityDataBuffer(0, BufferType::Dynamic)
	  , boneBuffer(MAX_BONES_PER_MODEL * sizeof(glm::mat4), BufferType::Dynamic)
	  , numUniqueEntities(0u)
	  , frameTime(0.0f)
	  , numInstances(0u)
	  , numDrawCalls(0u)
	  , numTrianglesSubmitted(0u)
	  , numTrianglesDrawn(0u)
	{ }
  };

  class GeometryPass : public RenderPass
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
                float id = 0.0f, bool drawSelectionMask = false);
    void submit(Model* data, Animator* animation, ModelMaterial& materials,
                const glm::mat4 &model, float id = 0.0f,
                bool drawSelectionMask = false);
  private:
	GeometryPassDataBlock passData;

	AsynchTimer timer;
  };
}