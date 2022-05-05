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

  struct GeomStaticDrawData
  {
	ShaderStorageBuffer* indexBuffer;
	ShaderStorageBuffer* vertexBuffer;
	Material* technique;

	uint numToRender;

	GeomStaticDrawData(ShaderStorageBuffer* indexBuffer, ShaderStorageBuffer* vertexBuffer, Material* technique, 
					   uint numToRender)
	  : indexBuffer(indexBuffer)
	  , vertexBuffer(vertexBuffer)
      , technique(technique)
	  , numToRender(numToRender)
	{ }

	bool operator==(const GeomStaticDrawData& other) const
	{ 
	  return this->indexBuffer == other.indexBuffer && this->vertexBuffer == other.vertexBuffer 
		     && this->technique == other.technique;
	}
  };

  struct PerEntityData
  {
	glm::mat4 transform;
	glm::vec4 idMask;
	Material::BlockData materialData;

	PerEntityData(const glm::mat4 &transform, const glm::vec4 &idMask,
				  const Material::BlockData& materialData)
	  : transform(transform)
	  , idMask(idMask)
	  , materialData(materialData)
	{ }
  };

  struct GeomDynamicDrawData
  {
	ShaderStorageBuffer* indexBuffer;
	ShaderStorageBuffer* vertexBuffer;
	Material* technique;
	Animator* animations;

	uint numToRender;

	PerEntityData data;

	uint instanceCount;

	GeomDynamicDrawData(ShaderStorageBuffer* indexBuffer, ShaderStorageBuffer* vertexBuffer, Material* technique, 
						uint numToRender, Animator* animations, const PerEntityData &data)
	  : indexBuffer(indexBuffer)
	  , vertexBuffer(vertexBuffer)
      , technique(technique)
	  , numToRender(numToRender)
	  , animations(animations)
	  , data(data)
	  , instanceCount(1)
	{ }
  };
}

template<>
struct std::hash<Strontium::GeomStaticDrawData>
{
  std::size_t operator()(Strontium::GeomStaticDrawData const &data) const noexcept
  {
    std::size_t h1 = std::hash<Strontium::ShaderStorageBuffer*>{}(data.indexBuffer);
	std::size_t h2 = std::hash<Strontium::ShaderStorageBuffer*>{}(data.vertexBuffer);
    std::size_t h3 = std::hash<Strontium::Material*>{}(data.technique);
    return h3 ^ (h2 << 1) ^ (h3 << 2);
  }
};

namespace Strontium
{
  struct GeometryPassDataBlock
  {
	// Required buffers and lists to draw stuff.
	GeometryBuffer gBuffer;
	FrameBuffer idMaskBuffer;

	Shader* staticGeometry;
	Shader* dynamicGeometry;
	Shader* staticEditor;
	Shader* dynamicEditor;

	UniformBuffer cameraBuffer;
	UniformBuffer perDrawUniforms;

	ShaderStorageBuffer entityDataBuffer;
	ShaderStorageBuffer boneBuffer;

	uint numUniqueEntities;
	robin_hood::unordered_flat_map<GeomStaticDrawData, std::vector<PerEntityData>> staticInstanceMap;
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
	  , staticGeometry(nullptr)
	  , dynamicGeometry(nullptr)
	  , staticEditor(nullptr)
	  , dynamicEditor(nullptr)
      , cameraBuffer(3 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), BufferType::Dynamic)
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