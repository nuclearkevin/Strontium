#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/AssetManager.h"
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
    Material(MaterialType type, const std::vector<std::pair<std::string, SciRenderer::AssetHandle>> &sampler2Ds);
    ~Material();

    // Attach a texture.
    void attachSampler2D(const std::string &samplerName, const SciRenderer::AssetHandle &handle);

    // Prepare for drawing.
    void configure();

    // Check to see if the material has a texture of a certain type.
    bool hasSampler2D(const std::string &samplerName);

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
    Texture2D* getSampler2D(const std::string &samplerName);

    // Operator overloading makes this nice and easy.
    operator Shader*() { return this->program; }

    // Get the internal storage for serialization.
    std::vector<std::pair<std::string, GLfloat>>& getFloats() { return this->floats; }
    std::vector<std::pair<std::string, glm::vec2>>& getVec2s() { return this->vec2s; }
    std::vector<std::pair<std::string, glm::vec3>>& getVec3s() { return this->vec3s; }
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>>& getSampler2Ds() { return this->sampler2Ds; }
  private:
    MaterialType type;

    // The shader and shader data.
    Shader* program;
    std::vector<std::pair<std::string, GLfloat>> floats;
    std::vector<std::pair<std::string, glm::vec2>> vec2s;
    std::vector<std::pair<std::string, glm::vec3>> vec3s;
    // First string is the name of the shader uniform. 2nd string is the internal
    // asset handle for the texture.
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>> sampler2Ds;
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

    std::vector<std::pair<Shared<Mesh>, Material>>& getStorage() { return this->materials; };
  private:
    std::vector<std::pair<Shared<Mesh>, Material>> materials;
  };
}
