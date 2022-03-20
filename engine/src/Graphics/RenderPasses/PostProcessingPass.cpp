#include "Graphics/RenderPasses/PostProcessingPass.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RendererCommands.h"

namespace Strontium
{
  PostProcessingPass::PostProcessingPass(Renderer3D::GlobalRendererData* globalRendererData,
                                         GeometryPass* previousGeoPass, BloomPass* previousBloomPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousBloomPass })
    , previousGeoPass(previousGeoPass)
    , previousBloomPass(previousBloomPass)
    , timer(5)
  { }

  PostProcessingPass::~PostProcessingPass()
  { }

  void 
  PostProcessingPass::onInit()
  {
    this->passData.postProcessingShader = ShaderCache::getShader("post_processing");
  }

  void 
  PostProcessingPass::updatePassData()
  { }

  RendererDataHandle 
  PostProcessingPass::requestRendererData()
  {
    return -1;
  }

  void 
  PostProcessingPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  PostProcessingPass::onRendererBegin(uint width, uint height)
  {
    this->passData.drawOutline = false;
  }

  void 
  PostProcessingPass::onRender()
  { }

  void 
  PostProcessingPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.begin();

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);

    // Bind the scene depth, entity IDs and mask.
    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    geometryBlock->gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
    geometryBlock->gBuffer.bindAttachment(FBOTargetParam::Colour3, 1);

    // Bind the lighting buffer.
    rendererData->lightingBuffer.bind(2);

    // Bind bloom texture.
    auto bloomPassBlock = this->previousBloomPass->getInternalDataBlock<BloomPassDataBlock>();
    if (bloomPassBlock->useBloom)
      bloomPassBlock->upsampleBuffer2.bind(3);

    // Bind the camera uniforms.
    this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->cameraBuffer.bindToPoint(0);

    // Upload the post processing settings.
    struct PostBlockData
    {
      glm::vec4 bloom;  // Bloom intensity (x) and radius (y). z and w are unused.
      glm::ivec2 postSettings; // Using bloom (bit 1), using FXAA (bit 2) (x). Tone mapping operator (y). z and w are unused.
    } 
      postBlock
    {
      { bloomPassBlock->intensity, bloomPassBlock->radius, 0.0f, 0.0f },
      { 0, static_cast<int>(this->passData.toneMapOp) }
    };
    if (this->passData.useFXAA)
      postBlock.postSettings.x |= (1 << 0);
    if (bloomPassBlock->useBloom)
      postBlock.postSettings.x |= (1 << 1);
    if (this->passData.useGrid)
      postBlock.postSettings.x |= (1 << 2);
    if (this->passData.drawOutline)
      postBlock.postSettings.x |= (1 << 3);

    this->passData.postProcessingParams.bindToPoint(1);
    this->passData.postProcessingParams.setData(0, sizeof(PostBlockData), &postBlock);

    RendererCommands::disable(RendererFunction::DepthTest);
    frontBuffer.setViewport();
    frontBuffer.bind();

    rendererData->blankVAO.bind();
    this->passData.postProcessingShader->bind();
    RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);

    frontBuffer.unbind();
    RendererCommands::enable(RendererFunction::DepthTest);

    this->timer.end();
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  PostProcessingPass::onShutdown()
  { }
}