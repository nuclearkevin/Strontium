#include "Graphics/Material.h"

// Project includes.
#include "Graphics/Buffers.h"

namespace Strontium
{
  Material::Material(MaterialType type, const std::string &filepath)
    : filepath(filepath)
    , type(type)
    , pipeline(false)
    , materialData(2 * sizeof(glm::vec4), BufferType::Dynamic)
  {
    switch (type)
    {
      case MaterialType::PBR:
      {
        this->program = ShaderCache::getShader("geometry_pass_shader");

        std::string texHandle;
        Texture2D::createMonoColour(glm::vec4(1.0f), texHandle);
        this->attachSampler2D("albedoMap", texHandle);
        this->attachSampler2D("metallicMap", texHandle);
        this->attachSampler2D("roughnessMap", texHandle);
        Texture2D::createMonoColour(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f), texHandle);
        this->attachSampler2D("normalMap", texHandle);
        Texture2D::createMonoColour(glm::vec4(1.0f), texHandle);
        this->attachSampler2D("aOcclusionMap", texHandle);
        this->attachSampler2D("specF0Map", texHandle);

        this->vec3s.emplace_back("uAlbedo", glm::vec3(1.0f));
        this->floats.emplace_back("uMetallic", 0.0f);
        this->floats.emplace_back("uRoughness", 0.5f);
        this->floats.emplace_back("uAO", 1.0f);
        this->floats.emplace_back("uEmiss", 0.0f);
        this->floats.emplace_back("uReflectance", 0.04f);

        this->materialData.bindToPoint(1);

        // Set the initial metallic, roughness, AO and emission parameters.
        auto mrae = glm::vec4(0.0f);
        mrae.x = this->getfloat("uMetallic");
        mrae.y = this->getfloat("uRoughness");
        mrae.z = this->getfloat("uAO");
        mrae.w = this->getfloat("uEmiss");
        this->materialData.setData(0, sizeof(glm::vec4), &mrae.x);

        // Set the initial albedo and F0 parameters.
        auto albedo = this->getvec3("uAlbedo");
        auto albedoF0 = glm::vec4(0.0f);
        albedoF0.x = albedo.x;
        albedoF0.y = albedo.y;
        albedoF0.z = albedo.z;
        albedoF0.w = this->getfloat("uReflectance");
        this->materialData.setData(sizeof(glm::vec4), sizeof(glm::vec4), &albedoF0.x);
        break;
      }
      default:
      {
        this->program = nullptr;
        break;
      }
    }
  }

  Material::~Material()
  { }

  void 
  Material::updateUniformBuffer()
  {
      // Set the metallic, roughness, AO and emission parameters.
      auto mrae = glm::vec4(0.0f);
      mrae.x = this->getfloat("uMetallic");
      mrae.y = this->getfloat("uRoughness");
      mrae.z = this->getfloat("uAO");
      mrae.w = this->getfloat("uEmiss");
      this->materialData.setData(0, sizeof(glm::vec4), &mrae.x);

      // Set the albedo and F0 parameters.
      auto albedo = this->getvec3("uAlbedo");
      auto albedoF0 = glm::vec4(0.0f);
      albedoF0.x = albedo.x;
      albedoF0.y = albedo.y;
      albedoF0.z = albedo.z;
      albedoF0.w = this->getfloat("uReflectance");
      this->materialData.setData(sizeof(glm::vec4), sizeof(glm::vec4), &albedoF0.x);
  }

  void
  Material::reflect()
  {
    /*
    auto& shaderUniforms = this->program->getUniforms();
    for (auto& pair : shaderUniforms)
    {
      switch (pair.second)
      {
        case UniformType::Float: this->floats.emplace_back(pair.first, 1.0f); break;
        case UniformType::Vec2: this->vec2s.emplace_back(pair.first, glm::vec2(1.0f)); break;
        case UniformType::Vec3: this->vec3s.emplace_back(pair.first, glm::vec3(1.0f)); break;
        case UniformType::Vec4: this->vec4s.emplace_back(pair.first, glm::vec4(1.0f)); break;
        case UniformType::Mat3: this->mat3s.emplace_back(pair.first, glm::mat3(1.0f)); break;
        case UniformType::Mat4: this->mat4s.emplace_back(pair.first, glm::mat4(1.0f)); break;
        case UniformType::Sampler1D: this->sampler1Ds.emplace_back(pair.first, "None"); break;
        case UniformType::Sampler2D: this->sampler2Ds.emplace_back(pair.first, "None"); break;
        case UniformType::Sampler3D: this->sampler3Ds.emplace_back(pair.first, "None"); break;
        case UniformType::SamplerCube: this->samplerCubes.emplace_back(pair.first, "None"); break;
      }
    }
    */
    // TODO: Reflection.
  }

  void
  Material::configure()
  {
    auto textureCache = AssetManager<Texture2D>::getManager();
    unsigned int samplerCount = 0;

    // Loop over 2D textures and assign them.
    // TODO: Other sampler types.
    for (auto& pair : this->sampler2Ds)
    {
      Texture2D* sampler = textureCache->getAsset(pair.second);

      this->program->addUniformSampler(pair.first.c_str(), samplerCount);
      if (sampler != nullptr)
        sampler->bind(samplerCount);

      samplerCount++;
    }

    this->materialData.bindToPoint(1);
  }

  void
  Material::configureDynamic(Shader* override)
  {
    auto textureCache = AssetManager<Texture2D>::getManager();
    unsigned int samplerCount = 0;

    // Loop over 2D textures and assign them.
    // TODO: Other sampler types.
    for (auto& pair : this->sampler2Ds)
    {
      Texture2D* sampler = textureCache->getAsset(pair.second);

      override->addUniformSampler(pair.first.c_str(), samplerCount);
      if (sampler != nullptr)
        sampler->bind(samplerCount);

      samplerCount++;
    }

    this->materialData.bindToPoint(1);
  }

  // Search for and attach textures to a sampler.
  bool
  Material::hasSampler1D(const std::string &samplerName)
  {
    return Utilities::pairSearch<std::string, std::string>(this->sampler1Ds, samplerName);
  }
  void
  Material::attachSampler1D(const std::string &samplerName, const Strontium::AssetHandle &handle)
  {
    if (!this->hasSampler1D(samplerName))
      this->sampler1Ds.push_back(std::pair(samplerName, handle));
    else
    {
      auto loc = Utilities::pairGet<std::string, std::string>(this->sampler1Ds, samplerName);

      this->sampler1Ds.erase(loc);
      this->sampler1Ds.push_back(std::pair(samplerName, handle));
    }
  }

  bool
  Material::hasSampler2D(const std::string &samplerName)
  {
    return Utilities::pairSearch<std::string, std::string>(this->sampler2Ds, samplerName);
  }
  void
  Material::attachSampler2D(const std::string &samplerName, const Strontium::AssetHandle &handle)
  {
    if (!this->hasSampler2D(samplerName))
      this->sampler2Ds.push_back(std::pair(samplerName, handle));
    else
    {
      auto loc = Utilities::pairGet<std::string, std::string>(this->sampler2Ds, samplerName);

      this->sampler2Ds.erase(loc);
      this->sampler2Ds.push_back(std::pair(samplerName, handle));
    }
  }

  bool
  Material::hasSampler3D(const std::string &samplerName)
  {
    return Utilities::pairSearch<std::string, std::string>(this->sampler3Ds, samplerName);
  }
  void
  Material::attachSampler3D(const std::string &samplerName, const Strontium::AssetHandle &handle)
  {
    if (!this->hasSampler3D(samplerName))
      this->sampler3Ds.push_back(std::pair(samplerName, handle));
    else
    {
      auto loc = Utilities::pairGet<std::string, std::string>(this->sampler3Ds, samplerName);

      this->sampler3Ds.erase(loc);
      this->sampler3Ds.push_back(std::pair(samplerName, handle));
    }
  }

  bool
  Material::hasSamplerCubemap(const std::string &samplerName)
  {
    return Utilities::pairSearch<std::string, std::string>(this->samplerCubes, samplerName);
  }
  void
  Material::attachSamplerCubemap(const std::string &samplerName, const Strontium::AssetHandle &handle)
  {
    if (!this->hasSamplerCubemap(samplerName))
      this->samplerCubes.push_back(std::pair(samplerName, handle));
    else
    {
      auto loc = Utilities::pairGet<std::string, std::string>(this->samplerCubes, samplerName);

      this->samplerCubes.erase(loc);
      this->samplerCubes.push_back(std::pair(samplerName, handle));
    }
  }

  Texture2D*
  Material::getSampler2D(const std::string &samplerName)
  {
    auto textureCache = AssetManager<Texture2D>::getManager();

    auto loc = Utilities::pairGet<std::string, std::string>(this->sampler2Ds, samplerName);

    return textureCache->getAsset(loc->second);
  }
  Strontium::AssetHandle&
  Material::getSampler2DHandle(const std::string &samplerName)
  {
    auto loc = Utilities::pairGet<std::string, std::string>(this->sampler2Ds, samplerName);
    return loc->second;
  }

  // Attach a mesh-material pair.
  void
  ModelMaterial::attachMesh(const std::string &meshName, MaterialType type)
  {
    auto materialAssets = AssetManager<Material>::getManager();

    if (!Utilities::pairSearch<std::string, AssetHandle>(this->materials, meshName))
    {
      materialAssets->attachAsset(meshName, new Material(type));
      this->materials.emplace_back(meshName, meshName);
    }
  }

  void
  ModelMaterial::attachMesh(const std::string &meshName, const AssetHandle &material)
  {
    if (!Utilities::pairSearch<std::string, AssetHandle>(this->materials, meshName))
      this->materials.emplace_back(meshName, material);
  }

  void
  ModelMaterial::swapMaterial(const std::string &meshName, const AssetHandle &newMaterial)
  {
    auto loc = Utilities::pairGet<std::string, AssetHandle>(this->materials, meshName);

    if (loc != this->materials.end())
      loc->second = newMaterial;
  }

  // Get a material given the mesh.
  Material*
  ModelMaterial::getMaterial(const std::string &meshName)
  {
    auto materialAssets = AssetManager<Material>::getManager();

    auto loc = Utilities::pairGet<std::string, AssetHandle>(this->materials, meshName);

    if (loc != this->materials.end())
      return materialAssets->getAsset(loc->second);

    return nullptr;
  }

  AssetHandle
  ModelMaterial::getMaterialHandle(const std::string &meshName)
  {
    auto loc = Utilities::pairGet<std::string, AssetHandle>(this->materials, meshName);

    if (loc != this->materials.end())
      return loc->second;

    return AssetHandle();
  }
}
