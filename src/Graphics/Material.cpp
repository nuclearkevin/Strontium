#include "Graphics/Material.h"

// Project includes.
#include "Core/AssetManager.h"

namespace SciRenderer
{
  Material::Material(MaterialType type)
    : type(type)
  {
    auto shaderCache = AssetManager<Shader>::getManager();
    switch (type)
    {
      case MaterialType::PBR:
      {
        this->program = shaderCache->getAsset("pbr_shader");

        std::string texHandle;
        Texture2D::createMonoColour(glm::vec4(1.0f), texHandle);
        this->sampler2Ds.push_back(std::make_pair("albedoMap", texHandle));
        this->sampler2Ds.push_back(std::make_pair("metallicMap", texHandle));
        this->sampler2Ds.push_back(std::make_pair("roughnessMap", texHandle));
        Texture2D::createMonoColour(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f), texHandle);
        this->sampler2Ds.push_back(std::make_pair("normalMap", texHandle));
        Texture2D::createMonoColour(glm::vec4(1.0f), texHandle);
        this->sampler2Ds.push_back(std::make_pair("aOcclusionMap", texHandle));

        this->vec3s.push_back(std::pair("uAlbedo", glm::vec3(1.0f)));
        this->floats.push_back(std::pair("uMetallic", 1.0f));
        this->floats.push_back(std::pair("uRoughness", 1.0f));
        this->floats.push_back(std::pair("uAO", 1.0f));
        break;
      }
      case MaterialType::Specular:
      {
        this->program = shaderCache->getAsset("specular_shader");
        break;
      }
      default:
      {
        this->program = nullptr;
        break;
      }
    }
  }

  Material::Material(MaterialType type, const std::vector<std::pair<std::string, SciRenderer::AssetHandle>> &sampler2Ds)
    : type(type)
    , sampler2Ds(sampler2Ds)
  { }

  Material::~Material()
  { }

  void
  Material::configure()
  {
    auto textureCache = AssetManager<Texture2D>::getManager();

    switch (this->type)
    {
      case MaterialType::PBR:
      {
        auto textureCache = AssetManager<Texture2D>::getManager();

        unsigned int samplerCount = 0;

        // Bind the PBR maps first.
        this->program->addUniformSampler("irradianceMap", 0);
      	this->program->addUniformSampler("reflectanceMap", 1);
      	this->program->addUniformSampler("brdfLookUp", 2);

        // Increase the sampler count to compensate.
        samplerCount += 3;

        // Loop over 2D textures and assign them.
        for (auto& pair : this->sampler2Ds)
        {
          Texture2D* sampler = textureCache->getAsset(pair.second);

          this->program->addUniformSampler(pair.first.c_str(), samplerCount);
          if (sampler != nullptr)
            sampler->bind(samplerCount);

          samplerCount++;
        }

        // TODO: Do other sampler types (1D textures, 3D textures, cubemaps).
        // Loop over the floats and assign them.
        for (auto& pair : this->floats)
          this->program->addUniformFloat(pair.first.c_str(), pair.second);

        // Loop over the vec2s and assign them.
        for (auto& pair : this->vec2s)
          this->program->addUniformVector(pair.first.c_str(), pair.second);

        // Loop over the vec3s and assign them.
        for (auto& pair : this->vec3s)
          this->program->addUniformVector(pair.first.c_str(), pair.second);

        break;
      }
      case MaterialType::Specular:
      {
        break;
      }
      case MaterialType::Unknown:
        break;
    }

    this->program->bind();
  }

  Texture2D*
  Material::getSampler2D(const std::string &samplerName)
  {
    auto textureCache = AssetManager<Texture2D>::getManager();

    auto loc = Utilities::pairGet<std::string, std::string>(this->sampler2Ds, samplerName);

    return textureCache->getAsset(loc->second);
  }

  // Attach a texture.
  void
  Material::attachSampler2D(const std::string &samplerName, const SciRenderer::AssetHandle &handle)
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

  // Check to see if the material has a texture of a certain type.
  bool
  Material::hasSampler2D(const std::string &samplerName)
  {
    return Utilities::pairSearch<std::string, std::string>(this->sampler2Ds, samplerName);
  }

  // Attach a mesh-material pair.
  void
  ModelMaterial::attachMesh(Shared<Mesh> mesh, MaterialType type)
  {
    this->materials.push_back(std::make_pair(mesh, Material(type)));
  }

  // Get a material given the mesh.
  Material*
  ModelMaterial::getMaterial(Shared<Mesh> mesh)
  {
    auto loc = Utilities::pairGet<Shared<Mesh>, Material>(this->materials, mesh);

    if (loc != this->materials.end())
      return &loc->second;

    return nullptr;
  }
}
