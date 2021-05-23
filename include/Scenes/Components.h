#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/Meshes.h"
#include "Graphics/Textures.h"

namespace SciRenderer
{
  // Entity nametag component.
  struct NameComponent
  {
    std::string name;
    std::string description;

    NameComponent(const NameComponent&) = default;

    NameComponent()
      : name("")
      , description("")
    { }

    NameComponent(const std::string &name, const std::string &description)
      : name(name)
      , description(description)
    { }

    friend std::ostream& operator<<(std::ostream& os, const NameComponent &component)
    {
      os << "Name: " << component.name << "\n" << "Description: "
         << component.description;
      return os;
    }

    operator std::string()
    {
      return "Name: " + name + "\n" + "Description: " + description;
    }
  };

  // Entity transform component.
  struct TransformComponent
  {
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale; // Euler angles. Pitch = x, yaw = y, roll = z.
    GLfloat scaleFactor;

    TransformComponent(const TransformComponent&) = default;

    TransformComponent()
      : translation(glm::vec3(0.0f))
      , rotation(glm::vec3(0.0f))
      , scale(glm::vec3(1.0f))
      , scaleFactor(1.0f)
    { }

    TransformComponent(const glm::vec3 &translation, const glm::vec3 &rotation,
                       const glm::vec3 &scale)
      : translation(translation)
      , rotation(rotation)
      , scale(scale)
      , scaleFactor(1.0f)
    { }

    // Cast to glm::mat4 for easy multiplication.
    operator glm::mat4()
    {
      return glm::translate(glm::mat4(1.0f), translation)
             * glm::toMat4(glm::quat(rotation)) * glm::scale(scale * scaleFactor);
    }
  };

  // Renderable component. This is the component which is passed to the renderer
  // alongside the transform component.
  struct RenderableComponent
  {
    Shared<Mesh> mesh;

    // TODO: Move these two to a material class and pass a material shared ptr instead.
    Shared<Shader> shader;
    std::vector<Shared<Texture2D>> textures;

    RenderableComponent(const RenderableComponent&) = default;

    RenderableComponent(const Shared<Mesh> &mesh, const Shared<Shader> &shader,
                        const std::vector<Shared<Texture2D>> &textures)
      : mesh(mesh)
      , shader(shader)
      , textures(textures)
    { }

    RenderableComponent(const Shared<Mesh> &mesh, const Shared<Shader> &shader)
      : mesh(mesh)
      , shader(shader)
    { }

    operator Shared<Mesh>()
    {
      return mesh;
    }

    operator Shared<Shader>()
    {
      return shader;
    }
  };
}
