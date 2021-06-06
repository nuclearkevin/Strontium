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

        this->sampler2Ds.push_back(std::make_pair("albedoMap", Texture2D::createMonoColour(glm::vec4(1.0f))));
        this->sampler2Ds.push_back(std::make_pair("metallicMap", Texture2D::createMonoColour(glm::vec4(1.0f))));
        this->sampler2Ds.push_back(std::make_pair("roughnessMap", Texture2D::createMonoColour(glm::vec4(1.0f))));
        this->sampler2Ds.push_back(std::make_pair("normalMap", Texture2D::createMonoColour(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f))));
        this->sampler2Ds.push_back(std::make_pair("aOcclusionMap", Texture2D::createMonoColour(glm::vec4(1.0f))));

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

  Material::Material(MaterialType type, const std::vector<std::pair<std::string, Texture2D*>> &sampler2Ds)
    : type(type)
    , sampler2Ds(sampler2Ds)
  { }

  Material::~Material()
  { }

  void
  Material::configure()
  {
    switch (this->type)
    {
      case MaterialType::PBR:
      {
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
          this->program->addUniformSampler(pair.first.c_str(), samplerCount);
          pair.second->bind(samplerCount);

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
  Material::getTexture2D(const std::string &name)
  {
    auto loc = Utilities::pairGet<std::string, Texture2D*>(this->sampler2Ds, name);

    return loc->second;
  }

  // Attach a texture.
  void
  Material::attachTexture2D(Texture2D* tex, const std::string &name)
  {
    if (!this->hasTexture2D(name))
      this->sampler2Ds.push_back(std::pair(name, tex));
    else
    {
      auto loc = Utilities::pairGet<std::string, Texture2D*>(this->sampler2Ds, name);

      this->sampler2Ds.erase(loc);
      this->sampler2Ds.push_back(std::pair(name, tex));
    }
  }

  // Check to see if the material has a texture of a certain type.
  bool
  Material::hasTexture2D(const std::string &name)
  {
    return Utilities::pairSearch<std::string, Texture2D*>(this->sampler2Ds, name);
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
