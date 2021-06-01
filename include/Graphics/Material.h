#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"

namespace SciRenderer
{
  // Type of the material.
  enum class MaterialType
  {
    PBR, Specular, Unknown
  };

  // Surface maps.
  enum class MaterialTexType
  {
    Albedo, Metallic, Roughness, Normal, AO, Unknown
  };

  class Material
  {
  public:
    Material(MaterialType type = MaterialType::PBR);
    Material(MaterialType type, const std::vector<std::pair<MaterialTexType, Texture2D*>> &textures);
    ~Material();

    // Attach a texture.
    void attachTexture(Texture2D* tex, const MaterialTexType &type);

    // Prepare for drawing.
    void configure();

    // Check to see if the material has a texture of a certain type.
    bool hasTexture(MaterialTexType type);

    MaterialType& getType() { return this->type; }
    Shader* getShader() { return this->program; }
    Texture2D* getTexture(MaterialTexType type);

    glm::vec3& getAlbedo() { return this->uAlbedo; }
    GLfloat& getMetallic() { return this->uMetallic; }
    GLfloat& getRoughness() { return this->uRoughness; }
    GLfloat& getAO() { return this->uAO; }
    bool& useMultiples() { return this->useUMultiples; }
  private:
    MaterialType type;

    std::vector<std::pair<MaterialTexType, Texture2D*>> textures;
    Shader* program;

    glm::vec3 uAlbedo;
    GLfloat uMetallic;
    GLfloat uRoughness;
    GLfloat uAO;

    bool useUMultiples;
  };
}
