#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/HBAOPass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"
#include "Graphics/GPUTimers.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  struct IBLApplicationPassDataBlock
  {
    Shader* iblEvaluation;
    Shader* brdfIntegration;
    
    Texture2D brdfLUT;

    UniformBuffer iblBuffer;

    // The actual IBL properties.
    std::vector<DynamicIBL> dynamicIBLParams;

    // Some statistics.
    float frameTime;
  
    IBLApplicationPassDataBlock()
      : iblEvaluation(nullptr)
      , brdfIntegration(nullptr)
      , iblBuffer(sizeof(glm::vec4), BufferType::Dynamic)
      , frameTime(0.0f)
    { }
  };

  class IBLApplicationPass : public RenderPass
  {
  public:
	IBLApplicationPass(Renderer3D::GlobalRendererData* globalRendererData, 
                       GeometryPass* previousGeoPass,
                       HBAOPass* previousHBAOPass,
					   DynamicSkyIBLPass* previousDynSkyIBLPass);
	~IBLApplicationPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

    void submitDynamicSkyIBL(const DynamicIBL& params);
  private:
    IBLApplicationPassDataBlock passData;

    GeometryPass* previousGeoPass;
    DynamicSkyIBLPass* previousDynSkyIBLPass;
    HBAOPass* previousHBAOPass;

    AsynchTimer timer;
  };
}