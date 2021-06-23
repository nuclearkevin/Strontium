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
    PBR, Specular, GeometryPass, Unknown
  };

  // Individual material class to hold shaders and shader data.
  class Material
  {
  public:
    Material(MaterialType type = MaterialType::PBR);
    ~Material();

    // Prepare for drawing.
    void configure();
    void configure(Shader* overrideProgram);

    // Sampler configuration.
    bool hasSampler1D(const std::string &samplerName);
    void attachSampler1D(const std::string &samplerName, const SciRenderer::AssetHandle &handle);
    bool hasSampler2D(const std::string &samplerName);
    void attachSampler2D(const std::string &samplerName, const SciRenderer::AssetHandle &handle);
    bool hasSampler3D(const std::string &samplerName);
    void attachSampler3D(const std::string &samplerName, const SciRenderer::AssetHandle &handle);
    bool hasSamplerCubemap(const std::string &samplerName);
    void attachSamplerCubemap(const std::string &samplerName, const SciRenderer::AssetHandle &handle);

    // TODO: Implement 1D and 3D texture fetching.
    //Texture1D* getSampler1D(const std::string &samplerName);
    Texture2D* getSampler2D(const std::string &samplerName);
    SciRenderer::AssetHandle& getSampler2DHandle(const std::string &samplerName);
    //Texture3D* getSampler1D(const std::string &samplerName);
    //CubeMap* getSamplerCubemap(const std::string &samplerName);

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
    glm::vec4& getVec4(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::vec4>(this->vec4s, name);
      return loc->second;
    };
    glm::mat3& getMat3(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::mat3>(this->mat3s, name);
      return loc->second;
    };
    glm::mat4& getMat4(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::mat4>(this->mat4s, name);
      return loc->second;
    };

    // Operator overloading makes this nice and easy.
    operator Shader*() { return this->program; }

    // Get the internal storage for serialization.
    std::vector<std::pair<std::string, GLfloat>>& getFloats() { return this->floats; }
    std::vector<std::pair<std::string, glm::vec2>>& getVec2s() { return this->vec2s; }
    std::vector<std::pair<std::string, glm::vec3>>& getVec3s() { return this->vec3s; }
    std::vector<std::pair<std::string, glm::vec4>>& getVec4s() { return this->vec4s; }
    std::vector<std::pair<std::string, glm::mat3>>& getMat3s() { return this->mat3s; }
    std::vector<std::pair<std::string, glm::mat4>>& getMat4s() { return this->mat4s; }
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>>& getSampler1Ds() { return this->sampler1Ds; }
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>>& getSampler2Ds() { return this->sampler2Ds; }
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>>& getSampler3Ds() { return this->sampler3Ds; }
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>>& getSamplerCubemaps() { return this->samplerCubes; }
  private:
    // Reflect the attached shader.
    void reflect();

    // The material type.
    MaterialType type;

    // The shader and shader data.
    Shader* program;
    std::vector<std::pair<std::string, GLfloat>> floats;
    std::vector<std::pair<std::string, glm::vec2>> vec2s;
    std::vector<std::pair<std::string, glm::vec3>> vec3s;
    std::vector<std::pair<std::string, glm::vec4>> vec4s;
    std::vector<std::pair<std::string, glm::mat3>> mat3s;
    std::vector<std::pair<std::string, glm::mat4>> mat4s;

    std::vector<std::pair<std::string, SciRenderer::AssetHandle>> sampler1Ds;
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>> sampler2Ds;
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>> sampler3Ds;
    std::vector<std::pair<std::string, SciRenderer::AssetHandle>> samplerCubes;
  };

  // Macro material which holds all the individual material objects for each
  // submesh of a model.
  class ModelMaterial
  {
  public:
    ModelMaterial() = default;
    ~ModelMaterial() = default;

    void attachMesh(const std::string &meshName, MaterialType type = MaterialType::PBR);
    void attachModel(Model* model, MaterialType type = MaterialType::PBR);

    Material* getMaterial(const std::string &meshName);

    // Get the storage.
    std::vector<std::pair<std::string, Material>>& getStorage() { return this->materials; };
  private:
    std::vector<std::pair<std::string, Material>> materials;
  };
}
