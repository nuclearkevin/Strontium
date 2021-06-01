#include "Graphics/Material.h"

// Project includes.
#include "Core/AssetManager.h"

namespace SciRenderer
{
  Material::Material(MaterialType type)
    : type(type)
    , uAlbedo(1.0f)
    , uMetallic(1.0f)
    , uRoughness(1.0f)
    , uAO(1.0f)
    , useUMultiples(false)
  {
    auto shaderCache = AssetManager<Shader>::getManager();
    switch (type)
    {
      case MaterialType::PBR: this->program = shaderCache->getAsset("pbr_shader"); break;
      case MaterialType::Specular: this->program = shaderCache->getAsset("specular_shader"); break;
      default: this->program = nullptr; break;
    }

    this->textures.push_back(std::make_pair(MaterialTexType::Albedo, Texture2D::createMonoColour(glm::vec4(1.0f))));
    this->textures.push_back(std::make_pair(MaterialTexType::Metallic, Texture2D::createMonoColour(glm::vec4(1.0f))));
    this->textures.push_back(std::make_pair(MaterialTexType::Roughness, Texture2D::createMonoColour(glm::vec4(1.0f))));
    this->textures.push_back(std::make_pair(MaterialTexType::Normal, Texture2D::createMonoColour(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f))));
    this->textures.push_back(std::make_pair(MaterialTexType::AO, Texture2D::createMonoColour(glm::vec4(1.0f))));
  }

  Material::Material(MaterialType type, const std::vector<std::pair<MaterialTexType, Texture2D*>> &textures)
    : type(type)
    , textures(textures)
    , uAlbedo(1.0f)
    , uMetallic(1.0f)
    , uRoughness(1.0f)
    , uAO(1.0f)
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
        for (auto& pair : this->textures)
        {
          switch (pair.first)
          {
            case MaterialTexType::Albedo:
            {
              this->program->addUniformSampler2D("albedoMap", 0);
              pair.second->bind(0);
              break;
            }
            case MaterialTexType::Metallic:
            {
              this->program->addUniformSampler2D("metallicMap", 1);
              pair.second->bind(1);
              break;
            }
            case MaterialTexType::Roughness:
            {
              this->program->addUniformSampler2D("roughnessMap", 2);
              pair.second->bind(2);
              break;
            }
            case MaterialTexType::Normal:
            {
              this->program->addUniformSampler2D("normalMap", 3);
              pair.second->bind(3);
              break;
            }
            case MaterialTexType::AO:
            {
              this->program->addUniformSampler2D("aOcclusionMap", 4);
              pair.second->bind(4);
              break;
            }
            default:
              break;
          }
        }
        this->program->addUniformSampler2D("irradianceMap", 5);
      	this->program->addUniformSampler2D("reflectanceMap", 6);
      	this->program->addUniformSampler2D("brdfLookUp", 7);

        if (this->useUMultiples)
        {
          this->program->addUniformVector("uAlbedo", this->uAlbedo);
          this->program->addUniformFloat("uMetallic", this->uMetallic);
          this->program->addUniformFloat("uRoughness", this->uRoughness);
          this->program->addUniformFloat("uAO", this->uAO);
        }
        else
        {
          this->program->addUniformVector("uAlbedo", glm::vec3(1.0f));
          this->program->addUniformFloat("uMetallic", 1.0f);
          this->program->addUniformFloat("uRoughness", 1.0f);
          this->program->addUniformFloat("uAO", 1.0f);
        }
        break;
      }
      case MaterialType::Specular:
      {
        break;
      }
      case MaterialType::Unknown:
        break;
    }
  }

  Texture2D*
  Material::getTexture(MaterialTexType type)
  {
    auto loc = std::find_if(this->textures.begin(), this->textures.end(),
                            [&type](const std::pair<MaterialTexType, Texture2D*> &pair)
    {
      return pair.first == type;
    });

    if (loc != this->textures.end())
      return loc->second;
    else
      return nullptr;
  }

  // Attach a texture.
  void
  Material::attachTexture(Texture2D* tex, const MaterialTexType &type)
  {
    if (!this->hasTexture(type))
      this->textures.push_back(std::pair(type, tex));
  }

  // Check to see if the material has a texture of a certain type.
  bool
  Material::hasTexture(MaterialTexType type)
  {
    auto loc = std::find_if(this->textures.begin(), this->textures.end(),
                            [&type](const std::pair<MaterialTexType, Texture2D*> &pair)
    {
      return pair.first == type;
    });

    return loc != this->textures.end();
  }
}
