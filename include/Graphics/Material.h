#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Meshes.h"
#include "Utils/Utilities.h"

namespace SciRenderer
{
  // Type of the material.
  enum class MaterialType
  {
    PBR, Specular, Unknown
  };

  // Individual material class to hold shaders and shader data.
  class Material
  {
  public:
    Material(MaterialType type = MaterialType::PBR);
    Material(MaterialType type, const std::vector<std::pair<std::string, Texture2D*>> &sampler2Ds);
    ~Material();

    // Attach a texture.
    void attachTexture2D(Texture2D* tex, const std::string &name);

    // Prepare for drawing.
    void configure();

    // Check to see if the material has a texture of a certain type.
    bool hasTexture2D(const std::string &name);

    // Get the shader type.
    MaterialType& getType() { return this->type; }

    // Get the shader program.
    Shader* getShader() { return this->program; }

    // Get the shader data.
    GLfloat& getFloat(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, GLfloat>(this->floats, name);
      return loc->second;
    };
    glm::vec2& getVec2(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::vec2>(this->vec2s, name);
      return loc->second;
    };
    glm::vec3& getVec3(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::vec3>(this->vec3s, name);
      return loc->second;
    };
    Texture2D* getTexture2D(const std::string &name);

    // Operator overloading makes this nice and easy.
    operator Shader*() { return this->program; }

  private:
    MaterialType type;

    // The shader and shader data.
    Shader* program;
    std::vector<std::pair<std::string, GLfloat>> floats;
    std::vector<std::pair<std::string, glm::vec2>> vec2s;
    std::vector<std::pair<std::string, glm::vec3>> vec3s;
    std::vector<std::pair<std::string, Texture2D*>> sampler2Ds;
  };

  // Macro material which holds all the individual material objects for each
  // submesh of a model.
  class ModelMaterial
  {
  public:
    ModelMaterial() = default;
    ~ModelMaterial() = default;

    void attachMesh(Shared<Mesh> mesh, MaterialType type = MaterialType::PBR);
    void attachModel(Model* model, MaterialType type = MaterialType::PBR);

    Material* getMaterial(Shared<Mesh> mesh);
  private:
    std::vector<std::pair<Shared<Mesh>, Material>> materials;
  };
}
