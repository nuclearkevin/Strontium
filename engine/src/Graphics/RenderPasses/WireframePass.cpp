#include "Graphics/RenderPasses/WireframePass.h"

#include "Graphics/Renderer.h"

namespace Strontium
{
  WireframePass::WireframePass(DebugRenderer::GlobalRendererData* globalRendererData, 
                               GeometryPass* previousGeoPass)
    : RenderPass(&this->passData, globalRendererData, { nullptr })
    , previousGeoPass(previousGeoPass)
    , timer(5)
  { }

  WireframePass::~WireframePass()
  { }
  
  void 
  WireframePass::onInit()
  {
    // TODO: line shader.

    this->passData.debugCube.load("./assets/.internal/debug/cube.gltf");
    auto& max = this->passData.debugCube.getMaxPos();
    auto& min = this->passData.debugCube.getMinPos();
    auto center = (max + min) / 2.0f;
    this->passData.debugCubeExtents = glm::max(glm::abs(max), glm::abs(min)) - center;

    this->passData.debugSphere.load("./assets/.internal/debug/sphere.gltf");
    this->passData.debugSphereRadius = glm::max(glm::length(this->passData.debugSphere.getMaxPos()), 
                                                glm::length(this->passData.debugSphere.getMinPos()));
  }
  
  void 
  WireframePass::updatePassData()
  {
    
  }
  
  RendererDataHandle 
  WireframePass::requestRendererData()
  {
    return -1;
  }
  
  void 
  WireframePass::deleteRendererData(RendererDataHandle& handle)
  { }
  
  void
  WireframePass::onRendererBegin(uint width, uint height)
  {
    this->passData.lines.clear();
    this->passData.sphereQueue.clear();
    this->passData.obbQueue.clear();
  }
  
  void
  WireframePass::onRender()
  {
  
  }
  
  void
  WireframePass::onRendererEnd(FrameBuffer& frontBuffer)
  {
  
  }
  
  void
  WireframePass::onShutdown()
  {
  
  }
  
  void 
  WireframePass::submitLine(const glm::vec3& pointOne, const glm::vec3& pointTwo, 
                            const glm::vec3 &colour)
  {
    this->passData.lines.emplace_back(pointOne, colour);
    this->passData.lines.emplace_back(pointTwo, colour);
  }
  
  void
  WireframePass::submitFrustum(const Frustum& frustum, const glm::vec3 &colour)
  {
    // Connecting lines between the near and far place vertices.
    for (uint i = 0; i < 4; i++)
      this->submitLine(frustum.corners[i], frustum.corners[i + 1], colour);

    // Near face of the frustum.
    this->submitLine(frustum.corners[0], frustum.corners[1], colour);
    this->submitLine(frustum.corners[3], frustum.corners[2], colour);
    this->submitLine(frustum.corners[0], frustum.corners[2], colour);
    this->submitLine(frustum.corners[1], frustum.corners[3], colour);

    // Far face of the frustum.
    this->submitLine(frustum.corners[4], frustum.corners[5], colour);
    this->submitLine(frustum.corners[7], frustum.corners[6], colour);
    this->submitLine(frustum.corners[4], frustum.corners[6], colour);
    this->submitLine(frustum.corners[5], frustum.corners[7], colour);
  }
  
  void
  WireframePass::submitSphere(const Sphere &sphere, const glm::vec3 &colour)
  {
    float modelRadius = glm::max(glm::length(this->passData.debugSphere.getMaxPos()), 
                                 glm::length(this->passData.debugSphere.getMinPos()));
    float scale = sphere.radius / this->passData.debugSphereRadius;

    this->passData.sphereQueue.emplace_back(glm::translate(sphere.center) * glm::scale(glm::vec3(scale)));
  }
  
  void
  WireframePass::submitOrientedBox(const OrientedBoundingBox &box, const glm::vec3 &colour)
  {
    auto extents = box.extents / this->passData.debugCubeExtents;

    this->passData.obbQueue.emplace_back(glm::translate(box.center) 
                                         * glm::toMat4(box.orientation) 
                                         * glm::scale(extents));
  }
}