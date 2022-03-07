#include "Graphics/Renderer.h"

// Project includes.
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/RenderPasses/HiZPass.h"
#include "Graphics/RenderPasses/HBAOPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"

#include "Graphics/RenderPasses/IBLApplicationPass.h"
#include "Graphics/RenderPasses/DirectionalLightPass.h"

#include "Graphics/RenderPasses/SkyboxPass.h"

#include "Graphics/RenderPasses/BloomPass.h"
#include "Graphics/RenderPasses/PostProcessingPass.h"

//----------------------------------------------------------------------------
// 3D renderer starts here.
//----------------------------------------------------------------------------
namespace Strontium::Renderer3D
{
  static RenderPassManager* passManager;
  static GlobalRendererData* rendererData;

  // Get the renderer storage, state and stats.
  GlobalRendererData& getStorage() { return (*rendererData); }
  RenderPassManager& getPassManager() { return (*passManager); }

  // Initialize the renderer.
  void
  init(const uint width, const uint height)
  {
    // Initialize OpenGL parameters.
    RendererCommands::enable(RendererFunction::DepthTest);
    RendererCommands::enable(RendererFunction::CubeMapSeamless);

    // Setup global storage which gets used by multiple passes.
    rendererData = new GlobalRendererData();

    rendererData->gamma = 2.2f;

    // The lighting pass buffer.
    Texture2DParams lightingParams = Texture2DParams();
    lightingParams.sWrap = TextureWrapParams::ClampEdges;
    lightingParams.tWrap = TextureWrapParams::ClampEdges;
    lightingParams.internal = TextureInternalFormats::RGBA16f;
    lightingParams.format = TextureFormats::RGBA;
    lightingParams.dataType = TextureDataType::Floats;
    rendererData->lightingBuffer.setSize(1600, 900);
    rendererData->lightingBuffer.setParams(lightingParams);
    rendererData->lightingBuffer.initNullTexture();

    // A half-res buffer for half-resolution effects.
    Texture2DParams halfRes1Params = Texture2DParams();
    halfRes1Params.sWrap = TextureWrapParams::ClampEdges;
    halfRes1Params.tWrap = TextureWrapParams::ClampEdges;
    halfRes1Params.internal = TextureInternalFormats::RGBA16f;
    halfRes1Params.format = TextureFormats::RGBA;
    halfRes1Params.dataType = TextureDataType::Floats;
    rendererData->halfResBuffer1.setSize(1600 / 2, 900 / 2);
    rendererData->halfResBuffer1.setParams(halfRes1Params);
    rendererData->halfResBuffer1.initNullTexture();

    // Initialize the renderpass system.
    passManager = new RenderPassManager();

    // Insert the render passes.
    // Pre-lighting passes.
    auto geomet = passManager->insertRenderPass<GeometryPass>(rendererData);
    auto shadow = passManager->insertRenderPass<ShadowPass>(rendererData);
    auto hiZ = passManager->insertRenderPass<HiZPass>(rendererData, geomet);
    auto hbao = passManager->insertRenderPass<HBAOPass>(rendererData, geomet, hiZ);
    auto skyatmo = passManager->insertRenderPass<SkyAtmospherePass>(rendererData, geomet);
    auto dynIBL = passManager->insertRenderPass<DynamicSkyIBLPass>(rendererData, skyatmo);

    // Lighting passes.
    auto iblApp = passManager->insertRenderPass<IBLApplicationPass>(rendererData, geomet, hbao, dynIBL);
    auto dirApp = passManager->insertRenderPass<DirectionalLightPass>(rendererData, geomet, shadow, skyatmo);

    // Skybox pass. This should be applied last.
    auto skyboxApp = passManager->insertRenderPass<SkyboxPass>(rendererData, geomet, skyatmo);

    // Post processing passes
    auto bloom = passManager->insertRenderPass<BloomPass>(rendererData);
    auto post = passManager->insertRenderPass<PostProcessingPass>(rendererData, geomet, bloom);

    // Init the render passes.
    passManager->onInit();
  }

  // Shutdown the renderer.
  void
  shutdown()
  {
    delete passManager;
    delete rendererData;
  }

  // Generic begin and end for the renderer.
  void
  begin(uint width, uint height, const Camera &sceneCamera)
  {
    // Set the camera.
    rendererData->sceneCam = sceneCamera;
    rendererData->camFrustum = buildCameraFrustum(sceneCamera);

    // Resize the global buffers.
    if (static_cast<uint>(rendererData->lightingBuffer.getWidth()) != width ||
        static_cast<uint>(rendererData->lightingBuffer.getHeight()) != height)
    {
      rendererData->lightingBuffer.setSize(width, height);
      rendererData->lightingBuffer.initNullTexture();
    }
    uint groupX = static_cast<uint>(glm::ceil(static_cast<float>(width) / 8.0f));
    uint groupY = static_cast<uint>(glm::ceil(static_cast<float>(height) / 8.0f));
    rendererData->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::Write);
    ShaderCache::getShader("clear_tex_2D")->launchCompute(groupX, groupY, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    uint hWidth = static_cast<uint>(glm::ceil(static_cast<float>(width) / 2.0f));
    uint hHeight = static_cast<uint>(glm::ceil(static_cast<float>(height) / 2.0f));
    if (static_cast<uint>(rendererData->halfResBuffer1.getWidth()) != hWidth ||
        static_cast<uint>(rendererData->halfResBuffer1.getHeight()) != hHeight)
	{
	  rendererData->halfResBuffer1.setSize(hWidth, hHeight);
	  rendererData->halfResBuffer1.initNullTexture();
	  rendererData->halfResBuffer1.generateMips();
	}

    // Prep the renderpasses.
    passManager->onRendererBegin(width, height);
  }

  void
  end(FrameBuffer& frontBuffer)
  {
    // Call the render functions since the Renderer basically does all its work when submission is done.
    passManager->onRender();

    // Whichever renderpass needs to do work when the rendering phase is over.
    passManager->onRendererEnd(frontBuffer);
  }
}
