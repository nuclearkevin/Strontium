#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/AssetManager.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Meshes.h"
#include "Utils/Utilities.h"

namespace Strontium
{
  // Type of the material.
  enum class MaterialType
  {
    PBR, Unknown
  };

  // Individual material class to hold shaders and shader data.
  class Material
  {
  public:
    Material(MaterialType type = MaterialType::PBR, const std::string &filepath = "");
    ~Material();

    // Prepare for drawing.
    void configure();

    // Sampler configuration.
    bool hasSampler1D(const std::string &samplerName);
    void attachSampler1D(const std::string &samplerName, const Strontium::AssetHandle &handle);
    bool hasSampler2D(const std::string &samplerName);
    void attachSampler2D(const std::string &samplerName, const Strontium::AssetHandle &handle);
    bool hasSampler3D(const std::string &samplerName);
    void attachSampler3D(const std::string &samplerName, const Strontium::AssetHandle &handle);
    bool hasSamplerCubemap(const std::string &samplerName);
    void attachSamplerCubemap(const std::string &samplerName, const Strontium::AssetHandle &handle);

    // TODO: Implement 1D and 3D texture fetching.
    //Texture1D* getSampler1D(const std::string &samplerName);
    Texture2D* getSampler2D(const std::string &samplerName);
    Strontium::AssetHandle& getSampler2DHandle(const std::string &samplerName);
    //Texture3D* getSampler1D(const std::string &samplerName);
    //CubeMap* getSamplerCubemap(const std::string &samplerName);

    // Get the filepath.
    std::string& getFilepath() { return this->filepath; }

    // Get the shader and pipeline type.
    MaterialType& getType() { return this->type; }
    bool& getPipeline() { return this->pipeline; }

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
    std::vector<std::pair<std::string, Strontium::AssetHandle>>& getSampler1Ds() { return this->sampler1Ds; }
    std::vector<std::pair<std::string, Strontium::AssetHandle>>& getSampler2Ds() { return this->sampler2Ds; }
    std::vector<std::pair<std::string, Strontium::AssetHandle>>& getSampler3Ds() { return this->sampler3Ds; }
    std::vector<std::pair<std::string, Strontium::AssetHandle>>& getSamplerCubemaps() { return this->samplerCubes; }
  private:
    // Reflect the attached shader.
    void reflect();

    // The location (if any) on disk.
    std::string filepath;

    // The material type and pipeline.
    MaterialType type;
    bool pipeline;

    // The shader and shader data.
    Shader* program;
    std::vector<std::pair<std::string, GLfloat>> floats;
    std::vector<std::pair<std::string, glm::vec2>> vec2s;
    std::vector<std::pair<std::string, glm::vec3>> vec3s;
    std::vector<std::pair<std::string, glm::vec4>> vec4s;
    std::vector<std::pair<std::string, glm::mat3>> mat3s;
    std::vector<std::pair<std::string, glm::mat4>> mat4s;

    std::vector<std::pair<std::string, Strontium::AssetHandle>> sampler1Ds;
    std::vector<std::pair<std::string, Strontium::AssetHandle>> sampler2Ds;
    std::vector<std::pair<std::string, Strontium::AssetHandle>> sampler3Ds;
    std::vector<std::pair<std::string, Strontium::AssetHandle>> samplerCubes;

    // The uniform buffer to store shader data in.
    UniformBuffer materialData;
  };

  // Macro material which holds all the individual material objects for each
  // submesh of a model.
  class ModelMaterial
  {
  public:
    ModelMaterial() = default;
    ~ModelMaterial() = default;

    void attachMesh(const std::string &meshName, MaterialType type = MaterialType::PBR);
    void attachMesh(const std::string &meshName, const AssetHandle &material);
    void swapMaterial(const std::string &meshName, const AssetHandle &newMaterial);

    Material* getMaterial(const std::string &meshName);
    AssetHandle getMaterialHandle(const std::string &meshName);
    GLuint getNumStored() { return this->materials.size(); }

    // Get the storage.
    std::vector<std::pair<std::string, AssetHandle>>& getStorage() { return this->materials; };
  private:
    std::vector<std::pair<std::string, AssetHandle>> materials;
  };
}
