#pragma once

#define NUM_CASCADES 4

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Math.h"
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Model.h"
#include "Graphics/Animations.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  namespace Renderer3D
  {
    struct GlobalRendererData;
  }

  struct ShadowMeshData
  {
	DrawArraysIndirectCommand drawData;

	std::vector<glm::mat4> instanceTransforms;

	ShadowMeshData(uint count, uint instanceCount, uint first, uint baseInstance)
	  : drawData(count, instanceCount, first, baseInstance)
	{ }
  };

  struct ShadowDynamicDrawData
  {
	uint numToRender;
	uint globalBufferOffset;

	Animator* animations;

	glm::mat4 transform;

	uint instanceCount;

	ShadowDynamicDrawData(uint globalBufferOffset, uint numToRender,
		                  Animator* animations,
					      const glm::mat4 &transform)
	  : globalBufferOffset(globalBufferOffset)
	  , numToRender(numToRender)
	  , animations(animations)
	  , transform(transform)
	  , instanceCount(1)
	{ }
  };
}

namespace Strontium
{
  struct ShadowPassDataBlock
  {
	FrameBuffer shadowBuffer;
	DrawIndirectBuffer indirectBuffer;

	Shader* staticShadow;
	Shader* dynamicShadow;

	// Shadow view-projection calculation information.
	float cascadeLambda;
	uint shadowMapRes;
	bool hasCascades;
	glm::vec4 cascadeSplits[NUM_CASCADES];

	// The primary light.
	bool castShadows;
	DirectionalLight primaryLight;
	// The pseudo-cameras for the shadow pass.
	glm::mat4 cascades[NUM_CASCADES];
	Frustum lightCullingFrustums[NUM_CASCADES];

	// Required buffers and lists to draw stuff.
	UniformBuffer lightSpaceBuffer;
	UniformBuffer perDrawUniforms;

	ShaderStorageBuffer transformBuffer;
	ShaderStorageBuffer drawIDToTransformMap;
	ShaderStorageBuffer boneBuffer;

	uint numUniqueEntities;
	uint numUniqueStaticMeshes;
	glm::vec3 minPos;
	glm::vec3 maxPos;
	robin_hood::unordered_flat_map<Model*, uint> modelMap;
	std::vector<ShadowMeshData> staticGeometry;
	std::vector<ShadowDynamicDrawData> dynamicDrawList;

	// Shadow settings.
	uint shadowQuality;
	bool useSSShadows;
	uint numSearchSteps;
	uint numPCFTaps;
	float minRadius;
	float blendFraction;
	float normalBias;
	float constBias;

	// Some statistics to display.
	float frameTime;
	uint numInstances;
	uint numDrawCalls;
	uint numTrianglesDrawn;

	ShadowPassDataBlock()
	  : staticShadow(nullptr)
	  , dynamicShadow(nullptr)
	  , cascadeLambda(0.5f)
	  , shadowMapRes(2048)
	  , hasCascades(false)
	  , cascadeSplits()
	  , cascades()
	  , lightCullingFrustums()
	  , castShadows(false)
	  , lightSpaceBuffer(sizeof(glm::mat4), BufferType::Dynamic)
	  , perDrawUniforms(sizeof(int), BufferType::Dynamic)
	  , transformBuffer(0u, BufferType::Dynamic)
	  , drawIDToTransformMap(0u, BufferType::Dynamic)
	  , boneBuffer(MAX_BONES_PER_MODEL * sizeof(glm::mat4), BufferType::Dynamic)
	  , indirectBuffer(0u, BufferType::Dynamic)
	  , numUniqueEntities(0u)
	  , numUniqueStaticMeshes(0u)
	  , minPos(std::numeric_limits<float>::max())
	  , maxPos(std::numeric_limits<float>::min())
	  , shadowQuality(0)
	  , useSSShadows(true)
	  , numSearchSteps(16u)
	  , numPCFTaps(64u)
	  , minRadius(1.0f)
	  , blendFraction(0.5f)
	  , normalBias(50.0f)
	  , constBias(0.01f)
	  , frameTime(0.0f)
	  , numInstances(0u)
	  , numDrawCalls(0u)
	  , numTrianglesDrawn(0u)
	{ }
  };

  class ShadowPass final : public RenderPass
  {
  public:
    ShadowPass(Renderer3D::GlobalRendererData* globalRendererData);
	~ShadowPass() override;

	void onInit() override;
	void updatePassData() override;
	RendererDataHandle requestRendererData() override;
	void deleteRendererData(RendererDataHandle& handle) override;
	void onRendererBegin(uint width, uint height) override;
	void onRender() override;
	void onRendererEnd(FrameBuffer& frontBuffer) override;
	void onShutdown() override;

	void submit(Model* data, const glm::mat4 &model);
	void submit(Model* data, Animator* animation, const glm::mat4& model);
	void submitPrimary(const DirectionalLight &primaryLight, bool castShadows, const glm::mat4 &model);
  private:
	void computeShadowData();

	ShadowPassDataBlock passData;

	AsynchTimer timer;
  };
}