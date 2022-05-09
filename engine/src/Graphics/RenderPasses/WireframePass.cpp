#include "Graphics/RenderPasses/WireframePass.h"

#include "Graphics/Renderer.h"
#include "Graphics/RendererCommands.h"

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
    this->passData.lineShader = ShaderCache::getShader("debug_lines");
    this->passData.wireframeShader = ShaderCache::getShader("debug_wireframe");
    this->passData.lineApplyShader = ShaderCache::getShader("apply_lines");

    this->passData.debugSphere.load("./assets/.internal/debug/sphere.gltf");
    for (auto& mesh : this->passData.debugSphere.getSubmeshes())
      mesh.init();

    this->passData.debugHalfSphere.load("./assets/.internal/debug/halfSphere.gltf");
    for (auto& mesh : this->passData.debugHalfSphere.getSubmeshes())
      mesh.init();

    this->passData.debugCube.load("./assets/.internal/debug/cube.gltf");
    for (auto& mesh : this->passData.debugCube.getSubmeshes())
      mesh.init();
    
    this->passData.debugCylinder.load("./assets/.internal/debug/cylinder.gltf");
    for (auto& mesh : this->passData.debugCylinder.getSubmeshes())
      mesh.init();

    this->passData.lineVAO.setData(this->passData.lineBuffer);
    this->passData.lineVAO.addAttribute(0, AttribType::Vec3, false, sizeof(LineVertex), 0);
    this->passData.lineVAO.addAttribute(1, AttribType::Vec3, false, sizeof(LineVertex), offsetof(LineVertex, colour));

    auto cSpec = Texture2D::getFloatColourParams();
    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    auto colourAttachment = FBOAttachment(FBOTargetParam::Colour0, FBOTextureParam::Texture2D,
                                          cSpec.internal, cSpec.format, cSpec.dataType);
    auto dSpec = Texture2D::getDefaultDepthParams();
    dSpec.sWrap = TextureWrapParams::ClampEdges;
    dSpec.tWrap = TextureWrapParams::ClampEdges;
    auto depthAttachment = FBOAttachment(FBOTargetParam::Depth, FBOTextureParam::Texture2D,
                                         dSpec.internal, dSpec.format, dSpec.dataType);

    this->passData.wireframeView.attach(cSpec, colourAttachment);
    this->passData.wireframeView.attach(dSpec, depthAttachment);
    this->passData.wireframeView.setClearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    RendererCommands::enable(RendererFunction::SmoothLines);
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
    glm::uvec2 gBufferSize = this->passData.wireframeView.getSize();
	if (width != gBufferSize.x || height != gBufferSize.y)
	  this->passData.wireframeView.resize(width, height);

    this->passData.lines.clear();
    this->passData.sphereQueue.clear();
    this->passData.halfSphereQueue.clear();
    this->passData.obbQueue.clear();
    this->passData.cylinderQueue.clear();
  }
  
  void
  WireframePass::onRender()
  {
    // Submit the line data.
    this->passData.lineBuffer->resize(sizeof(LineVertex) * this->passData.lines.size(), 
                                      BufferType::Dynamic);
    this->passData.lineBuffer->setData(0, sizeof(LineVertex) * this->passData.lines.size(), this->passData.lines.data());

    // Bind the camera buffer.
    static_cast<DebugRenderer::GlobalRendererData*>(this->globalBlock)->cameraBuffer.bindToPoint(0);

    this->passData.wireframeView.clear();
    this->passData.wireframeView.setViewport();
    this->passData.wireframeView.bind();

    // Draw the submitted lines.
    if (this->passData.lines.size() > 0u)
    {
      this->passData.lineShader->bind();
      this->passData.lineVAO.bind();
      this->passData.lineBuffer->bind();
      RendererCommands::drawArrays(PrimativeType::Line, 0, this->passData.lines.size());
      this->passData.lineBuffer->unbind();
      this->passData.lineVAO.unbind();
    }

    this->passData.wireframeShader->bind();
    RendererCommands::setPolygonMode(PolygonMode::Line);
    this->passData.instancedData.bindToPoint(2);

    static_cast<DebugRenderer::GlobalRendererData*>(this->globalBlock)->blankVAO.bind();

    // Render spheres.
    if (this->passData.sphereQueue.size() > 0u)
    {
      this->passData.instancedData.resize(sizeof(WireframeData) * this->passData.sphereQueue.size(), 
                                          BufferType::Dynamic);
      this->passData.instancedData.setData(0, sizeof(WireframeData) * this->passData.sphereQueue.size(), 
                                           this->passData.sphereQueue.data());
      
      for (auto& submesh : this->passData.debugSphere.getSubmeshes())
      {
        if (!submesh.isDrawable())
          continue;

        submesh.getVertexBuffer()->bindToPoint(0);
        submesh.getIndexBuffer()->bindToPoint(1);

        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, 0, submesh.numToRender(),
                                              this->passData.sphereQueue.size());
      }
    }

    // Render half spheres.
    if (this->passData.halfSphereQueue.size() > 0u)
    {
      this->passData.instancedData.resize(sizeof(WireframeData) * this->passData.halfSphereQueue.size(),
                                          BufferType::Dynamic);
      this->passData.instancedData.setData(0, sizeof(WireframeData) * this->passData.halfSphereQueue.size(),
                                           this->passData.halfSphereQueue.data());
      
      for (auto& submesh : this->passData.debugHalfSphere.getSubmeshes())
      {
        if (!submesh.isDrawable())
          continue;

        submesh.getVertexBuffer()->bindToPoint(0);
        submesh.getIndexBuffer()->bindToPoint(1);

        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, 0, submesh.numToRender(),
                                              this->passData.halfSphereQueue.size());
      }
    }

    // Render boxes.
    if (this->passData.obbQueue.size() > 0u)
    {
      this->passData.instancedData.resize(sizeof(WireframeData) * this->passData.obbQueue.size(), 
                                          BufferType::Dynamic);
      this->passData.instancedData.setData(0, sizeof(WireframeData) * this->passData.obbQueue.size(),
                                           this->passData.obbQueue.data());
      
      for (auto& submesh : this->passData.debugCube.getSubmeshes())
      {
        if (!submesh.isDrawable())
          continue;

        submesh.getVertexBuffer()->bindToPoint(0);
        submesh.getIndexBuffer()->bindToPoint(1);

        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, 0, submesh.numToRender(),
                                              this->passData.obbQueue.size());
      }
    }

    if (this->passData.cylinderQueue.size() > 0u)
    {
      this->passData.instancedData.resize(sizeof(WireframeData) * this->passData.cylinderQueue.size(),
                                          BufferType::Dynamic);
      this->passData.instancedData.setData(0, sizeof(WireframeData) * this->passData.cylinderQueue.size(),
                                           this->passData.cylinderQueue.data());

      for (auto& submesh : this->passData.debugCylinder.getSubmeshes())
      {
        if (!submesh.isDrawable())
          continue;

        submesh.getVertexBuffer()->bindToPoint(0);
        submesh.getIndexBuffer()->bindToPoint(1);

        RendererCommands::drawArraysInstanced(PrimativeType::Triangle, 0, submesh.numToRender(),
                                              this->passData.cylinderQueue.size());
      }
    }

    RendererCommands::setPolygonMode(PolygonMode::Fill);
    this->passData.wireframeShader->unbind();

    this->passData.wireframeView.unbind();
  }
  
  void
  WireframePass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    auto rendererData = static_cast<DebugRenderer::GlobalRendererData*>(this->globalBlock);

    // Bind the scene depth.
    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    geometryBlock->gBuffer.bindAttachment(FBOTargetParam::Depth, 0);

    this->passData.wireframeView.bindTextureID(FBOTargetParam::Colour0, 1);
    this->passData.wireframeView.bindTextureID(FBOTargetParam::Depth, 2);

    RendererCommands::enable(RendererFunction::Blending);
    RendererCommands::blendEquation(BlendEquation::Additive);
    RendererCommands::blendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
    RendererCommands::disable(RendererFunction::DepthTest);
    frontBuffer.setViewport();
    frontBuffer.bind();

    rendererData->blankVAO.bind();
    this->passData.lineApplyShader->bind();
    RendererCommands::drawArrays(PrimativeType::Triangle, 0, 3);

    frontBuffer.unbind();
    RendererCommands::enable(RendererFunction::DepthTest);
    RendererCommands::disable(RendererFunction::Blending);
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
    this->submitLine(frustum.corners[0], frustum.corners[4], colour);
    this->submitLine(frustum.corners[1], frustum.corners[5], colour);
    this->submitLine(frustum.corners[2], frustum.corners[6], colour);
    this->submitLine(frustum.corners[3], frustum.corners[7], colour);

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
    float scale = sphere.radius;
    scale += 0.01f * scale;

    this->passData.sphereQueue.emplace_back(glm::translate(sphere.center) * glm::scale(glm::vec3(scale)), 
                                            glm::vec4(colour, 1.0f));
  }
  
  void 
  WireframePass::submitHalfSphere(const Sphere &sphere, const glm::quat &orientation, const glm::vec3 &colour)
  {
    float scale = sphere.radius;
    scale += 0.01f * scale;

    this->passData.halfSphereQueue.emplace_back(glm::translate(sphere.center) * glm::toMat4(orientation) 
                                            * glm::scale(glm::vec3(scale)), 
                                            glm::vec4(colour, 1.0f));
  }

  void
  WireframePass::submitOrientedBox(const OrientedBoundingBox &box, const glm::vec3 &colour)
  {
    auto extents = box.extents;
    extents += 0.01f * extents;

    this->passData.obbQueue.emplace_back(glm::translate(box.center) 
                                         * glm::toMat4(box.orientation) 
                                         * glm::scale(extents), 
                                         glm::vec4(colour, 1.0f));
  }

  void 
  WireframePass::submitCylinder(const Cylinder& cylinder, const glm::vec3& colour)
  {
    auto size = glm::vec3(cylinder.radius, cylinder.halfHeight, cylinder.radius);
    size += 0.01f * size;

    this->passData.cylinderQueue.emplace_back(glm::translate(cylinder.center)
                                              * glm::toMat4(cylinder.orientation)
                                              * glm::scale(size),
                                              glm::vec4(colour, 1.0f));
  }

  void 
  WireframePass::submitCapsule(const Capsule &capsule, const glm::vec3 &colour)
  {
    this->submitCylinder(Cylinder(capsule.center, capsule.halfHeight, 
                                  capsule.radius, capsule.orientation), colour);

    auto rotQuat = capsule.orientation * glm::quat(glm::vec3(glm::radians(180.0f), 0.0f, 0.0f));

    auto rot = glm::toMat4(capsule.orientation);
    auto sphereCenter = capsule.center + glm::vec3(rot * glm::vec4(glm::vec3(0.0f, capsule.halfHeight, 0.0f), 1.0f));
    this->submitHalfSphere(Sphere(sphereCenter, capsule.radius), capsule.orientation, colour);
    sphereCenter = capsule.center - glm::vec3(rot * glm::vec4(glm::vec3(0.0f, capsule.halfHeight, 0.0f), 1.0f));
    this->submitHalfSphere(Sphere(sphereCenter, capsule.radius), rotQuat, colour);
  }
}