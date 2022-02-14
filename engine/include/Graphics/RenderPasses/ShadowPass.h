#pragma once

#define NUM_CASCADES 4

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Math.h"
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Model.h"
#include "Graphics/Animations.h"

namespace Strontium
{
  struct ShadowStaticDrawData
  {
	VertexArray* primatives;

	std::vector<glm::mat4> instancedTransforms;

	ShadowStaticDrawData(VertexArray* primatives)
	  : primatives(primatives)
	{ }

	operator VertexArray*() { return this->primatives; }
  };

  struct ShadowDynamicDrawData
  {
	VertexArray* primatives;
	Animator* animations;

	glm::mat4 transform;

	uint instanceCount;

	ShadowDynamicDrawData(VertexArray* primatives, Animator* animations,
					      const glm::mat4 & transform)
	  : primatives(primatives)
	  , animations(animations)
	  , transform(transform)
	  , instanceCount(1)
	{ }

	operator VertexArray*() { return this->primatives; }
	operator Animator* () { return this->animations; }
  };

  struct ShadowPassDataBlock
  {
	FrameBuffer shadowBuffers[NUM_CASCADES];

	Shader* staticShadow;
	Shader* dynamicShadow;

	// Shadow view-projection calculation information.
	float cascadeLambda;
	uint shadowMapRes;
	bool hasCascades;
	glm::vec4 cascadeSplits[NUM_CASCADES];

	// The pseudo-cameras for the shadow pass.
	glm::mat4 cascades[NUM_CASCADES];
	Frustum lightCullingFrustums[NUM_CASCADES];

	// Required buffers and lists to draw stuff.
	UniformBuffer lightSpaceBuffer;
	UniformBuffer perDrawUniforms;

	ShaderStorageBuffer transformBuffer;
	ShaderStorageBuffer boneBuffer;

	std::vector<ShadowStaticDrawData> staticDrawList;
	std::vector<ShadowDynamicDrawData> dynamicDrawList;

	// Shadow settings.
	uint shadowQuality;

	// Some statistics to display.
	float cpuTime;
	uint numInstances;
	uint numDrawCalls;
	uint numTrianglesSubmitted;
	uint numTrianglesDrawn;

	ShadowPassDataBlock()
	  : staticShadow(nullptr)
	  , dynamicShadow(nullptr)
	  , cascadeLambda(0.5f)
	  , shadowMapRes(2048)
	  , hasCascades(false)
	  , lightSpaceBuffer(sizeof(glm::mat4), BufferType::Dynamic)
	  , perDrawUniforms(sizeof(int), BufferType::Dynamic)
	  , transformBuffer(0, BufferType::Dynamic)
	  , boneBuffer(MAX_BONES_PER_MODEL * sizeof(glm::mat4), BufferType::Dynamic)
	  , shadowQuality(0)
	  , cpuTime(0.0f)
	  , numInstances(0u)
	  , numDrawCalls(0u)
	  , numTrianglesSubmitted(0u)
	  , numTrianglesDrawn(0u)
	{ }
  };

  class ShadowPass : public RenderPass
  {
  public:
    ShadowPass(Renderer3D::GlobalRendererData* globalRendererData);
	~ShadowPass() override;

	void onInit() override;
	void updatePassData() override;
	RendererDataHandle requestRendererData() override;
	void deleteRendererData(const RendererDataHandle& handle) override;
	void onRendererBegin(uint width, uint height) override;
	void onRender() override;
	void onRendererEnd(FrameBuffer& frontBuffer) override;
	void onShutdown() override;
  private:
	void computeShadowData();

	ShadowPassDataBlock passData;
  };
}