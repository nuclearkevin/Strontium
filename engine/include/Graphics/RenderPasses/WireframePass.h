#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Buffers.h"
#include "Graphics/VertexArray.h"
#include "Graphics/GPUTimers.h"

#include "Core/Math.h"

namespace Strontium
{
  namespace DebugRenderer
  {
    struct GlobalRendererData;
  }

  struct LineVertex
  {
    glm::vec3 position;
    glm::vec3 colour;

    LineVertex(const glm::vec3 &position, const glm::vec3 &colour)
      : position(position)
      , colour(colour)
    { }
  };

  struct WireframeData
  {
    glm::mat4 transform;
    glm::vec4 colour;

    WireframeData(const glm::mat4 &transform, const glm::vec4 &colour)
      : transform(transform)
      , colour(colour)
    { }
  };

  struct DebugPassDataBlock
  {
    Shader* lineShader;
    Shader* wireframeShader;
    Shader* lineApplyShader;

    Shared<VertexBuffer> lineBuffer;
    VertexArray lineVAO;

    ShaderStorageBuffer instancedData;

    FrameBuffer wireframeView;

    Model debugCube;
    Model debugSphere;
    Model debugCylinder;
    // TODO: Cylinder

    std::vector<LineVertex> lines;

    std::vector<WireframeData> sphereQueue;
    std::vector<WireframeData> obbQueue;
    std::vector<WireframeData> cylinderQueue;
    // TODO: Capsule

    DebugPassDataBlock()
      : lineShader(nullptr)
      , wireframeShader(nullptr)
      , lineApplyShader(nullptr)
      , lineBuffer(createShared<VertexBuffer>(BufferType::Dynamic))
      , instancedData(0, BufferType::Dynamic)
      , wireframeView(1600, 900)
      , debugCube()
      , debugSphere()
      , debugCylinder()
    { }
  };

  // Debug pass to draw simple shapes and lines.
  class WireframePass final : public RenderPass
  {
  public:
	WireframePass(DebugRenderer::GlobalRendererData* globalRendererData, 
                  GeometryPass* previousGeoPass);
	~WireframePass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle &handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer &frontBuffer) override;
    void onShutdown() override;

    void submitLine(const glm::vec3 &pointOne, const glm::vec3 &pointTwo, const glm::vec3 &colour);
    void submitFrustum(const Frustum &frustum, const glm::vec3 &colour);
    void submitSphere(const Sphere &sphere, const glm::vec3 &colour);
    void submitOrientedBox(const OrientedBoundingBox &box, const glm::vec3 &colour);
    void submitCylinder(const Cylinder &cylinder, const glm::vec3 &colour);
  private:
    DebugPassDataBlock passData;

    GeometryPass* previousGeoPass;

    AsynchTimer timer;
  };
}