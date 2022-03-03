#include "Graphics/RenderPasses/DirectionalLightPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  DirectionalLightPass::DirectionalLightPass(Renderer3D::GlobalRendererData* globalRendererData, 
                                             GeometryPass* previousGeoPass, 
                                             ShadowPass* previousShadowPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousShadowPass })
    , timer(5)
    , previousGeoPass(previousGeoPass)
    , previousShadowPass(previousShadowPass)
  { }

  DirectionalLightPass::~DirectionalLightPass()
  { }

  void 
  DirectionalLightPass::onInit()
  {

  }

  void 
  DirectionalLightPass::updatePassData()
  { }

  RendererDataHandle 
  DirectionalLightPass::requestRendererData()
  {
    return -1;
  }

  void 
  DirectionalLightPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  DirectionalLightPass::onRendererBegin(uint width, uint height)
  {
    this->passData.directionalLightCount = 0u;
  }

  void 
  DirectionalLightPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);
  }

  void 
  DirectionalLightPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  DirectionalLightPass::onShutdown()
  {

  }

  void 
  DirectionalLightPass::submit(const DirectionalLight &light, bool primaryLight, 
                               bool castShadows, const glm::mat4& model)
  {
    auto invTrans = glm::transpose(glm::inverse(model));
    DirectionalLight temp = light;
    temp.direction = glm::vec4(-1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), 0.0f);

    if (primaryLight)
    {
      this->passData.primaryLight = temp;
      this->passData.castShadows = castShadows;
    }
    else
      this->passData.directionalLightQueue[this->passData.directionalLightCount] = temp;

    this->passData.directionalLightCount++;
  }
}