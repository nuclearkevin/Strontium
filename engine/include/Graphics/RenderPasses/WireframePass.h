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

  struct DebugPassDataBlock
  {
    Shader* lineShader;
    Shader* wireframeShader;

    Model debugCube;
    glm::vec3 debugCubeExtents;

    Model debugSphere;
    float debugSphereRadius;
    // TODO: Capsule
    // TODO: Cylinder

    std::vector<LineVertex> lines;

    std::vector<glm::mat4> sphereQueue;
    std::vector<glm::mat4> obbQueue;
    // TODO: Capsule
    // TODO: Cylinder

    DebugPassDataBlock()
      : lineShader(nullptr)
      , wireframeShader(nullptr)
      , debugCube()
      , debugCubeExtents(1.0f)
      , debugSphere()
      , debugSphereRadius(1.0f)
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
  private:
    DebugPassDataBlock passData;

    GeometryPass* previousGeoPass;

    AsynchTimer timer;
  };
}