#include "Widgets/LightWidgets.h"

// ImGuizmo includes.
#include "imguizmo/ImGuizmo.h"

namespace Strontium
{
  LightWidget::LightWidget(const std::string &widgetName)
    : widgetName(widgetName)
    , dirLightWidget(ShaderCache::getShader("dir_widget"))
  {
    auto params = Texture2D::getFloatColourParams();

    this->buffer.setSize(128, 128);
    this->buffer.setParams(params);
    this->buffer.initNullTexture();
  }

  LightWidget::~LightWidget()
  { }

  void 
  LightWidget::dirLightManip(glm::mat4& transform, const ImVec2& size,
                             const ImVec2& uv1, const ImVec2& uv2)
  {
    // Resize the buffer if necessary.
    if (static_cast<int>(size.x) != this->buffer.getWidth() || 
        static_cast<int>(size.y) != this->buffer.getHeight())
    {
      this->buffer.setSize(size.x, size.y);
      this->buffer.initNullTexture();
    }

    ImGui::PushID(this->widgetName.c_str());
    ImGui::Text(this->widgetName.c_str());

    auto viewPos = glm::vec3(2.0f);
    auto viewDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - viewPos);
    auto view = glm::lookAt(viewPos, viewPos + viewDir, glm::vec3(0.0f, 1.0f, 0.0f));
    auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto invVP = glm::inverse(projection * view);

    auto lightDirection = glm::vec3(transform * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));

    this->dirLightWidget->addUniformMatrix("u_invVP", invVP, false);
    this->dirLightWidget->addUniformVector("u_camPos", viewPos);
    this->dirLightWidget->addUniformVector("u_lightDir", lightDirection);

    this->buffer.bindAsImage(0, 0, ImageAccessPolicy::Write);

    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getWidth())
                                                / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getHeight())
                                                 / 8.0f));
    this->dirLightWidget->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    auto widgetWidth = 0.75f * ImGui::GetWindowSize().x;
    ImGui::BeginChild("LightDirection", ImVec2(widgetWidth, widgetWidth));
    ImGui::Image(reinterpret_cast<ImTextureID>(buffer.getID()),
                 size, uv1, uv2);

    // ImGuizmo boilerplate. Prepare the drawing context and set the window to
    // draw the gizmos to.
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    auto windowMin = ImGui::GetWindowContentRegionMin();
    auto windowMax = ImGui::GetWindowContentRegionMax();
    auto windowOffset = ImGui::GetWindowPos();
    ImVec2 bounds[2];
    bounds[0] = ImVec2(windowMin.x + windowOffset.x,
                       windowMin.y + windowOffset.y);
    bounds[1] = ImVec2(windowMax.x + windowOffset.x,
                       windowMax.y + windowOffset.y);

    ImGuizmo::SetRect(bounds[0].x - 100.0f, bounds[0].y - 100.0f,
                      (bounds[1].x - bounds[0].x) + 200.0f,
                      (bounds[1].y - bounds[0].y) + 200.0f);

    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                         ImGuizmo::ROTATE, ImGuizmo::WORLD,
                         glm::value_ptr(transform), nullptr, nullptr);

    ImGui::EndChild();
    ImGui::PopID();
  }
}