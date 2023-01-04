#include "Graphics/RenderPasses/LightCullingPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  LightCullingPass::LightCullingPass(Renderer3D::GlobalRendererData* globalRendererData, 
                                     GeometryPass* previousGeoPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass })
    , previousGeoPass(previousGeoPass)
    , timer(5u)
  { }

  LightCullingPass::~LightCullingPass()
  { }

  void 
  LightCullingPass::onInit()
  {
    this->passData.buildTileData = ShaderCache::getShader("tiled_frustums_aabbs");
    this->passData.cullPointLights = ShaderCache::getShader("tiled_point_light_culling");
    this->passData.cullSpotLights = ShaderCache::getShader("tiled_spot_light_culling");
    this->passData.cullRectLights = ShaderCache::getShader("tiled_rect_light_culling");

    this->passData.tiles = static_cast<glm::uvec2>(glm::ceil(glm::vec2(1600.0f, 900.0f) / 16.0f));
    const uint numTiles = this->passData.tiles.x * this->passData.tiles.y;
    this->passData.cullingData.resize(numTiles * 8u * sizeof(glm::vec4), BufferType::Dynamic);

    // The light lists.
    this->passData.pointLightListBuffer.resize(numTiles * MAX_NUM_POINT_LIGHTS * sizeof(int), BufferType::Dynamic);
    this->passData.spotLightListBuffer.resize(numTiles * MAX_NUM_SPOT_LIGHTS * sizeof(int), BufferType::Dynamic);
    this->passData.rectLightListBuffer.resize(numTiles * MAX_NUM_RECT_LIGHTS * sizeof(int), BufferType::Dynamic);
  }

  void 
  LightCullingPass::updatePassData()
  { }

  RendererDataHandle 
  LightCullingPass::requestRendererData()
  {
    return -1;
  }

  void 
  LightCullingPass::deleteRendererData(RendererDataHandle &handle)
  { }

  void 
  LightCullingPass::onRendererBegin(uint width, uint height)
  {
    uint fWidth = static_cast<uint>(glm::ceil(static_cast<float>(width) / 16.0f));
    uint fHeight = static_cast<uint>(glm::ceil(static_cast<float>(height) / 16.0f));
    uint numTiles = fWidth * fHeight;
    if (numTiles != (this->passData.tiles.x * this->passData.tiles.y))
    {
      this->passData.tiles = glm::uvec2(static_cast<uint>(fWidth), static_cast<uint>(fHeight));
      const uint numTiles = static_cast<uint>(this->passData.tiles.x) * static_cast<uint>(this->passData.tiles.y);
      this->passData.cullingData.resize(numTiles * 8u * sizeof(glm::vec4), BufferType::Dynamic);
      
      this->passData.pointLightListBuffer.resize(numTiles * MAX_NUM_POINT_LIGHTS * sizeof(int), BufferType::Dynamic);
      this->passData.spotLightListBuffer.resize(numTiles * MAX_NUM_SPOT_LIGHTS * sizeof(int), BufferType::Dynamic);
      this->passData.rectLightListBuffer.resize(numTiles * MAX_NUM_RECT_LIGHTS * sizeof(int), BufferType::Dynamic);
    }

    this->passData.pointLightCount = 0u;
    this->passData.spotLightCount = 0u;
    this->passData.rectAreaLightCount = 0u;
  }

  void 
  LightCullingPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    uint numLights = this->passData.pointLightCount;
    numLights += this->passData.spotLightCount;
    numLights += this->passData.rectAreaLightCount;
    if (numLights == 0u)
      return;

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the GBuffer depth.
    auto& gBuffer = geometryBlock->gBuffer;
    gBuffer.bindAttachment(FBOTargetParam::Depth, 0);

    // Bind the camera block.
    static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock)->cameraBuffer.bindToPoint(0);

    // Bind the tile data SSBO.
    this->passData.cullingData.bindToPoint(0);

    // Launch the shader which generates the light culling data.
    this->passData.buildTileData->launchCompute(this->passData.tiles.x, this->passData.tiles.y, 1u);
    Shader::memoryBarrier(MemoryBarrierType::ShaderStorageBufferWrites);

    // Cull point lights.
    if (this->passData.pointLightCount > 0u)
    {
      // Bind the point light list.
      this->passData.pointLightListBuffer.bindToPoint(2);
      
      // Set the data for the point lights and bind them.
      this->passData.pointLights.setData(0u, MAX_NUM_POINT_LIGHTS * sizeof(PointLight),
                                         this->passData.pointLightQueue.data());
      this->passData.pointLights.setData(MAX_NUM_POINT_LIGHTS * sizeof(PointLight), sizeof(uint),
                                         &this->passData.pointLightCount);
      this->passData.pointLights.bindToPoint(1u);
      
      // Launch the shader which generates the culled light light.
      this->passData.cullPointLights->launchCompute(this->passData.tiles.x, this->passData.tiles.y, 1u);
      Shader::memoryBarrier(MemoryBarrierType::ShaderStorageBufferWrites);
    }

    // Cull spot lights.
    if (this->passData.spotLightCount > 0u)
    {
      // Bind the spot light list.
      this->passData.spotLightListBuffer.bindToPoint(2);
      
      // Set the data for the point lights and bind them.
      this->passData.spotLights.setData(0u, MAX_NUM_SPOT_LIGHTS * sizeof(SpotLight),
                                        this->passData.spotLightQueue.data());
      this->passData.spotLights.setData(MAX_NUM_SPOT_LIGHTS * sizeof(SpotLight), sizeof(uint),
                                        &this->passData.spotLightCount);
      this->passData.spotLights.bindToPoint(1u);

      // Launch the shader which generates the culled light light.
      this->passData.cullSpotLights->launchCompute(this->passData.tiles.x, this->passData.tiles.y, 1u);
      Shader::memoryBarrier(MemoryBarrierType::ShaderStorageBufferWrites);
    }
    
    // Cull rectangular area lights.
    if (this->passData.rectAreaLightCount > 0u)
    {
      // Bind the point light list.
      this->passData.rectLightListBuffer.bindToPoint(2);

      // Set the data for the rectangular area lights and bind them.
      this->passData.rectLights.setData(0u, this->passData.rectAreaLightCount * sizeof(RectAreaLight),
                                        this->passData.rectLightQueue.data());
      int count = static_cast<int>(this->passData.rectAreaLightCount);
      this->passData.rectLights.setData(MAX_NUM_RECT_LIGHTS * sizeof(RectAreaLight), sizeof(int), &count);
      this->passData.rectLights.bindToPoint(1u);

      // Launch the shader which generates the culled light light.
      this->passData.cullRectLights->launchCompute(this->passData.tiles.x, this->passData.tiles.y, 1u);
      Shader::memoryBarrier(MemoryBarrierType::ShaderStorageBufferWrites);
    }
  }

  void 
  LightCullingPass::onRendererEnd(FrameBuffer &frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  LightCullingPass::onShutdown()
  { }

  void 
  LightCullingPass::submit(const PointLight &light, const glm::mat4 &model)
  {
    if (this->passData.pointLightCount >= MAX_NUM_POINT_LIGHTS)
      return;

    this->passData.pointLightQueue[this->passData.pointLightCount].colourIntensity = light.colourIntensity;
    this->passData.pointLightQueue[this->passData.pointLightCount].positionRadius
        = glm::vec4(glm::vec3(model * glm::vec4(0.0, 0.0, 0.0, 1.0)), light.positionRadius.w);

    this->passData.pointLightCount++;
  }

  void 
  LightCullingPass::submit(const SpotLight &light, const glm::mat4 &model)
  {
    if (this->passData.spotLightCount >= MAX_NUM_SPOT_LIGHTS)
      return;

    this->passData.spotLightQueue[this->passData.spotLightCount].colourIntensity = light.colourIntensity;
    this->passData.spotLightQueue[this->passData.spotLightCount].cutOffs = light.cutOffs;
    this->passData.spotLightQueue[this->passData.spotLightCount].positionRange
        = glm::vec4(glm::vec3(model * glm::vec4(0.0, 0.0, 0.0, 1.0)), light.positionRange.w);

    auto invTrans = glm::transpose(glm::inverse(model));
    this->passData.spotLightQueue[this->passData.spotLightCount].direction 
        = glm::vec4(glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), 0.0f);

    /*
    float4 boundingSphere(in float3 origin, in float3 forward, in float size, in float angle)
{
    float4 boundingSphere;
    if(angle > PI/4.0f)
    {
        boundingSphere.xyz = origin + cos(angle) * size * forward;
        boundingSphere.w   = sin(angle) * size;
    }
    else
    {
        boundingSphere.xyz = origin + size / (2.0f * cos(angle)) * forward;
        boundingSphere.w   = size / (2.0f * cos(angle));
    }
 
    return boundingSphere;
}
    */
    float angle = glm::acos(light.cutOffs.y);
    if (angle > 0.25f * M_PI)
    {
      auto center = glm::vec3(this->passData.spotLightQueue[this->passData.spotLightCount].positionRange) 
          + light.cutOffs.y * light.positionRange.w * glm::vec3(this->passData.spotLightQueue[this->passData.spotLightCount].direction);
      float radius = glm::sin(angle) * light.positionRange.w;
      this->passData.spotLightQueue[this->passData.spotLightCount].cullingSphere = glm::vec4(center, radius);
    }
    else
    {
      auto center = glm::vec3(this->passData.spotLightQueue[this->passData.spotLightCount].positionRange) 
          + light.positionRange.w / (2.0f * light.cutOffs.y) * glm::vec3(this->passData.spotLightQueue[this->passData.spotLightCount].direction);
      float radius = light.positionRange.w / (2.0f * glm::cos(angle));
      this->passData.spotLightQueue[this->passData.spotLightCount].cullingSphere = glm::vec4(center, radius);
    }

    this->passData.spotLightCount++;
  }

  void 
  LightCullingPass::submit(const RectAreaLight &light, const glm::mat4 &model, float radius, 
                           bool twoSided, bool cull)
  {
    if (this->passData.rectAreaLightCount >= MAX_NUM_RECT_LIGHTS)
      return;

    this->passData.rectLightQueue[this->passData.rectAreaLightCount] = light;
    for (uint i = 0u; i < 4; ++i)
    {
      this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[i] = 
        model * this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[i];
    }
    this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[0].w = static_cast<float>(twoSided);
    this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[1].w = radius;
    this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[3].w = static_cast<float>(cull);

    this->passData.rectAreaLightCount++;
  }
}