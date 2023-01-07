#include "Graphics/Renderer.h"

// Project includes.
#include "Graphics/RendererCommands.h"

#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/RenderPasses/HiZPass.h"
#include "Graphics/RenderPasses/HBAOPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"

#include "Graphics/RenderPasses/LightCullingPass.h"

#include "Graphics/RenderPasses/GodrayPass.h"
#include "Graphics/RenderPasses/VolumetricLightPass.h"

#include "Graphics/RenderPasses/IBLApplicationPass.h"
#include "Graphics/RenderPasses/DirectionalLightPass.h"
#include "Graphics/RenderPasses/CulledLightingPass.h"
#include "Graphics/RenderPasses/AreaLightPass.h"

#include "Graphics/RenderPasses/SkyboxPass.h"

#include "Graphics/RenderPasses/BloomPass.h"
#include "Graphics/RenderPasses/PostProcessingPass.h"

#include "Graphics/RenderPasses/WireframePass.h"

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
    Logs::log("Initializing the 3D renderer.");

    // Initialize OpenGL parameters.
    RendererCommands::enable(RendererFunction::DepthTest);
    RendererCommands::enable(RendererFunction::CubeMapSeamless);
    RendererCommands::enable(RendererFunction::CullFaces);

    // Setup global storage which gets used by multiple passes.
    rendererData = new GlobalRendererData();

    rendererData->gamma = 2.2f;

    // Some noise textures.
    rendererData->spatialBlueNoise.reset(Texture2D::loadTexture2D("./assets/.internal/noise/HDR_RGBA_0.png", Texture2DParams(), false));
    rendererData->temporalBlueNoise.reset(Texture2D::loadTexture2D("./assets/.internal/noise/HDR_RGBA_1.png", Texture2DParams(), false));

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

    // A full-res buffer for full resolution effects.
    rendererData->fullResBuffer1.setSize(1600, 900);
    rendererData->fullResBuffer1.setParams(halfRes1Params);
    rendererData->fullResBuffer1.initNullTexture();

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

    // Light culling.
    auto lightCull = passManager->insertRenderPass<LightCullingPass>(rendererData, geomet);

    // Lighting passes.
    auto iblApp = passManager->insertRenderPass<IBLApplicationPass>(rendererData, geomet, hiZ, hbao, dynIBL);
    auto dirApp = passManager->insertRenderPass<DirectionalLightPass>(rendererData, geomet, shadow, skyatmo);
    auto culledApp = passManager->insertRenderPass<CulledLightingPass>(rendererData, geomet, lightCull);
    auto areaApp = passManager->insertRenderPass<AreaLightPass>(rendererData, geomet, lightCull);

    // Skybox pass. This should be applied right before the volumetrics but after everything else.
    auto skyboxApp = passManager->insertRenderPass<SkyboxPass>(rendererData, geomet, skyatmo);

    // Unified volumetric passes.
    auto ssgr = passManager->insertRenderPass<GodrayPass>(rendererData, geomet, lightCull, shadow, skyatmo, hiZ, dirApp, areaApp);
    auto volApp = passManager->insertRenderPass<VolumetricLightPass>(rendererData, geomet, hiZ, ssgr);

    // Post processing passes
    auto bloom = passManager->insertRenderPass<BloomPass>(rendererData);
    auto post = passManager->insertRenderPass<PostProcessingPass>(rendererData, geomet, bloom);

    // Init the render passes.
    passManager->onInit();

    // Init the debug renderer.
    DebugRenderer::init(1600u, 900u);
  }

  // Shutdown the renderer.
  void
  shutdown()
  {
    DebugRenderer::shutdown();
    Logs::log("Shutting down the 3D renderer.");

    delete passManager;
    delete rendererData;
  }

  // Add a mesh to the vertex and index cache. It's just a stretchy buffer at the moment.
  // TODO: smarter allocation scheme?
  uint
  addMeshToCache(Mesh &mesh)
  {
    auto& vertices = mesh.getData();
    uint newVertData = vertices.size() * sizeof(PackedVertex);
    auto& indices = mesh.getIndices();
    uint newIndexData = indices.size() * sizeof(uint);

    // First grow and populate the vertex cache.
    uint prevVertSize = rendererData->vertexCache.size();
    uint prevNumVertices = prevVertSize / sizeof(PackedVertex);
    if (prevVertSize > 0u)
    {
      rendererData->transferBuffer.resize(prevVertSize, BufferType::Static);
      rendererData->transferBuffer.copyDataFromSource(rendererData->vertexCache, 0u, 0u, prevVertSize);

      rendererData->vertexCache.resize(prevVertSize + newVertData, BufferType::Static);
      rendererData->vertexCache.copyDataFromSource(rendererData->transferBuffer, 0u, 0u, prevVertSize);
      rendererData->vertexCache.setData(prevVertSize, newVertData, vertices.data());
    }
    else
    {
      rendererData->vertexCache.resize(newVertData, BufferType::Static);
      rendererData->vertexCache.setData(0u, newVertData, vertices.data());
    }

    // Modify the indices for the base mesh to account for mashing all indices together into a single buffer.
    uint prevIndexSize = rendererData->indexCache.size();
    uint prevNumIndices = prevIndexSize / sizeof(uint);
    for (uint i = 0u; i < indices.size(); ++i)
      indices[i] += prevNumVertices;

    // Second: grow and populate the index cache.
    if (prevIndexSize > 0u)
    {
      rendererData->transferBuffer.resize(prevIndexSize, BufferType::Static);
      rendererData->transferBuffer.copyDataFromSource(rendererData->indexCache, 0u, 0u, prevIndexSize);

      rendererData->indexCache.resize(prevIndexSize + newIndexData, BufferType::Static);
      rendererData->indexCache.copyDataFromSource(rendererData->transferBuffer, 0u, 0u, prevIndexSize);
      rendererData->indexCache.setData(prevIndexSize, newIndexData, indices.data());
    }
    else
    {
      rendererData->indexCache.resize(newIndexData, BufferType::Static);
      rendererData->indexCache.setData(0u, newIndexData, indices.data());
    }

    rendererData->transferBuffer.resize(0u, BufferType::Static);

    return prevNumIndices;
  }

  // Generic begin and end for the renderer.
  void
  begin(uint width, uint height, const Camera &sceneCamera, float dt)
  {
    // Set the previous frame's data.
    rendererData->previousCamera = rendererData->sceneCam;
    rendererData->previousCamFrustum = rendererData->camFrustum;

    // Set the camera.
    rendererData->sceneCam = sceneCamera;
    rendererData->camFrustum = buildCameraFrustum(sceneCamera);

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
      rendererData->sceneCam.view,
      rendererData->sceneCam.projection,
      rendererData->sceneCam.invViewProj,
      { rendererData->sceneCam.position, 0.0 },
      { rendererData->sceneCam.near,
        rendererData->sceneCam.far, rendererData->gamma, 0.0 }
	};
    rendererData->cameraBuffer.setData(0, sizeof(CameraBlockData), &cameraBlock);

    // Parameters for temporal integration.
    rendererData->time++;
    if (rendererData->time > 511u)
     rendererData->time = 0u;

    // Upload data required for temporal AA.
    struct TemporalBlockData
    {
      glm::mat4 previousView;
      glm::mat4 previousProj;
      glm::mat4 previousVP;
      glm::mat4 previousInvVP;
      glm::vec4 previousPosTime;
    }
      temporalBlock
    {
      rendererData->previousCamera.view,
      rendererData->previousCamera.projection,
      rendererData->previousCamera.projection * rendererData->previousCamera.view,
      rendererData->previousCamera.invViewProj,
      { rendererData->previousCamera.position, static_cast<float>(rendererData->time) }
    };
    rendererData->temporalBuffer.setData(0, sizeof(TemporalBlockData), &temporalBlock);

    // Resize the global buffers.
    if (static_cast<uint>(rendererData->lightingBuffer.getWidth()) != width ||
        static_cast<uint>(rendererData->lightingBuffer.getHeight()) != height)
    {
      rendererData->lightingBuffer.setSize(width, height);
      rendererData->lightingBuffer.initNullTexture();

      rendererData->fullResBuffer1.setSize(width, height);
      rendererData->fullResBuffer1.initNullTexture();
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

//----------------------------------------------------------------------------
// Debug renderer starts here.
//----------------------------------------------------------------------------
namespace Strontium::DebugRenderer
{
  static RenderPassManager* passManager;
  static GlobalRendererData* rendererData;

  GlobalRendererData& 
  getStorage()
  {
    return *rendererData;
  }

  RenderPassManager& 
  getPassManager()
  {
    return *passManager;
  }
  
  // Init the renderer for drawing.
  void
  init(const uint width, const uint height)
  {
    Logs::log("Initializing the debug renderer.");

    // Setup global storage which gets used by multiple passes.
    rendererData = new GlobalRendererData();

    // Initialize the renderpass system.
    passManager = new RenderPassManager();

    auto geometry = Renderer3D::passManager->getRenderPass<GeometryPass>();
    auto wireframe = passManager->insertRenderPass<WireframePass>(rendererData, geometry);
    
    // Init the render passes.
    passManager->onInit();
  }

  void 
  shutdown()
  {
    Logs::log("Shutting down the debug renderer.");

    delete rendererData;
    delete passManager;
  }
  
  // Generic begin and end for the renderer.
  void 
  begin(uint width, uint height, const Camera &sceneCamera)
  {
    rendererData->sceneCam = sceneCamera;

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
      rendererData->sceneCam.view,
      rendererData->sceneCam.projection,
      rendererData->sceneCam.invViewProj,
      { rendererData->sceneCam.position, 0.0 },
      { rendererData->sceneCam.near,
        rendererData->sceneCam.far, 2.2f, 0.0 }
	};
    rendererData->cameraBuffer.setData(0, sizeof(CameraBlockData), &cameraBlock);

    // Prep the renderpasses.
    passManager->onRendererBegin(width, height);
  }

  void 
  end(FrameBuffer &frontBuffer)
  {
    // Call the render functions since the Renderer basically does all its work when submission is done.
    passManager->onRender();

    // Whichever renderpass needs to do work when the rendering phase is over.
    passManager->onRendererEnd(frontBuffer);
  }
}