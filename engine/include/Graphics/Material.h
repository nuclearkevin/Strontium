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

    void updateUniformBuffer();

    // Prepare for drawing.
    void configure();
    void configureDynamic(Shader* override);

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
    Texture2D* getSampler2D(const std::string &samplerName);
    Strontium::AssetHandle& getSampler2DHandle(const std::string &samplerName);

    // Get the filepath.
    std::string& getFilepath() { return this->filepath; }

    // Get the shader and pipeline type.
    MaterialType& getType() { return this->type; }

    // Get the shader program.
    Shader* getShader() { return this->program; }

    // Get the shader data.
    float getfloat(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, float>(this->floats, name);
      return loc->second;
    };

    glm::vec2 getvec2(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::vec2>(this->vec2s, name);
      return loc->second;
    };

    glm::vec3 getvec3(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::vec3>(this->vec3s, name);
      return loc->second;
    };

    glm::vec4 getvec4(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::vec4>(this->vec4s, name);
      return loc->second;
    };

    glm::mat3 getmat3(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::mat3>(this->mat3s, name);
      return loc->second;
    };

    glm::mat4 getmat4(const std::string &name)
    {
      auto loc = Utilities::pairGet<std::string, glm::mat4>(this->mat4s, name);
      return loc->second;
    };

    void set(float newFloat, const std::string& name)
    {
      auto loc = Utilities::pairGet<std::string, float>(this->floats, name);
      if (loc->second != newFloat)
      {
          loc->second = newFloat;
          this->updateUniformBuffer();
      }
    }

    void set(const glm::vec2 &newVec2, const std::string& name)
    {
        auto loc = Utilities::pairGet<std::string, glm::vec2>(this->vec2s, name);
        if (loc->second != newVec2)
        {
            loc->second = newVec2;
            this->updateUniformBuffer();
        }
    }

    void set(const glm::vec3& newVec3, const std::string& name)
    {
        auto loc = Utilities::pairGet<std::string, glm::vec3>(this->vec3s, name);
        if (loc->second != newVec3)
        {
            loc->second = newVec3;
            this->updateUniformBuffer();
        }
    }

    void set(const glm::vec4& newVec4, const std::string& name)
    {
        auto loc = Utilities::pairGet<std::string, glm::vec4>(this->vec4s, name);
        if (loc->second != newVec4)
        {
            loc->second = newVec4;
            this->updateUniformBuffer();
        }
    }

    void set(const glm::mat3& newMat3, const std::string& name)
    {
        auto loc = Utilities::pairGet<std::string, glm::mat3>(this->mat3s, name);
        if (loc->second != newMat3)
        {
            loc->second = newMat3;
            this->updateUniformBuffer();
        }
    }

    void set(const glm::mat4& newMat4, const std::string& name)
    {
        auto loc = Utilities::pairGet<std::string, glm::mat4>(this->mat4s, name);
        if (loc->second != newMat4)
        {
            loc->second = newMat4;
            this->updateUniformBuffer();
        }
    }

    // Operator overloading makes this nice and easy.
    operator Shader*() { return this->program; }

    // Get the internal storage for serialization.
    std::vector<std::pair<std::string, float>>& getFloats() { return this->floats; }
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
    std::vector<std::pair<std::string, float>> floats;
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
    uint getNumStored() { return this->materials.size(); }

    // Get the storage.
    std::vector<std::pair<std::string, AssetHandle>>& getStorage() { return this->materials; };
  private:
    std::vector<std::pair<std::string, AssetHandle>> materials;
  };
}
