#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/Meshes.h"

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
  };

  // Entity transform component.
  struct TransformComponent
  {
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale; // Euler angles. Pitch = x, yaw = y, roll = z.

    TransformComponent(const TransformComponent&) = default;

    TransformComponent()
      : translation(glm::vec3(0.0f))
      , rotation(glm::vec3(0.0f))
      , scale(glm::vec3(1.0f))
    { }

    TransformComponent(const glm::vec3 &translation, const glm::vec3 &rotation,
                       const glm::vec3 &scale)
      : translation(translation)
      , rotation(rotation)
      , scale(scale)
    { }

    // Cast to glm::mat4 for easy multiplication.
    operator glm::mat4()
    {
      return glm::translate(glm::mat4(1.0f), translation)
             * glm::toMat4(glm::quat(rotation)) * glm::scale(scale);
    }
  };

  // Entity mesh component.
  struct MeshComponent
  {
    Mesh* mesh;
  };

  // Entity material component. TODO: figure out material system and come back
  // to this.
  struct MaterialComponent
  {
    Shader* shader; // This is temporary.
  };


}
