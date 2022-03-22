#include "Serialization/YamlSerialization.h"

// Project includes.
#include "Scenes/Components.h"
#include "Scenes/Entity.h"
#include "Utils/AsyncAssetLoading.h"

#include "Graphics/RenderPasses/RenderPassManager.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/RenderPasses/HBAOPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"
#include "Graphics/RenderPasses/BloomPass.h"
#include "Graphics/RenderPasses/PostProcessingPass.h"
#include "Graphics/Renderer.h"

// YAML includes.
#include "yaml-cpp/yaml.h"

namespace YAML
{
  template <>
	struct convert<glm::vec2>
	{
	  static Node encode(const glm::vec2& rhs)
	  {
	    Node node;
	    node.push_back(rhs.x);
	    node.push_back(rhs.y);
           
	    node.SetStyle(EmitterStyle::Flow);
           
	    return node;
	  }
      
	  static bool decode(const Node& node, glm::vec2& rhs)
	  {
	    if (!node.IsSequence() || node.size() != 2)
	    	return false;
           
	    rhs.x = node[0].as<float>();
	    rhs.y = node[1].as<float>();
           
	    return true;
	  }
	};

  template <>
  struct convert<glm::vec3>
  {
    static Node encode(const glm::vec3& rhs)
    {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
            
      node.SetStyle(EmitterStyle::Flow);
            
      return node;
    }
       
    static bool decode(const Node& node, glm::vec3& rhs)
    {
      if (!node.IsSequence() || node.size() != 3)
      	return false;
            
      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
            
      return true;
    }
  };
  
  template <>
  struct convert<glm::vec4>
  {
    static Node encode(const glm::vec4& rhs)
    {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
      node.push_back(rhs.w);
            
      node.SetStyle(EmitterStyle::Flow);
            
      return node;
    }
  
    static bool decode(const Node& node, glm::vec4& rhs)
    {
      if (!node.IsSequence() || node.size() != 4)
      	return false;
            
      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
      rhs.w = node[3].as<float>();
            
      return true;
    }
  };
}

namespace Strontium
{
  namespace YAMLSerialization
  {
    YAML::Emitter& 
    operator<<(YAML::Emitter& out, const glm::vec2& v)
    {
      out << YAML::Flow;
      out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
      return out;
    }

    YAML::Emitter& 
    operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
      out << YAML::Flow;
      out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
      return out;
    }

    YAML::Emitter& 
    operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
      out << YAML::Flow;
      out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
      return out;
    }

    void
    serializeMaterial(YAML::Emitter &out, const AssetHandle &materialHandle, bool override = false)
    {
      auto textureCache = AssetManager<Texture2D>::getManager();
      auto materialAssets = AssetManager<Material>::getManager();

      auto material = materialAssets->getAsset(materialHandle);

      out << YAML::BeginMap;

      // Save the material name.
      out << YAML::Key << "MaterialName" << YAML::Value << materialHandle;
      if (material->getFilepath() != "" && override)
      {
        out << YAML::Key << "MaterialPath" << YAML::Value << material->getFilepath();
        out << YAML::EndMap;
        return;
      }

      // Save the material type.
      if (material->getType() == MaterialType::PBR)
        out << YAML::Key << "MaterialType" << YAML::Value << "pbr_shader";
      else
        out << YAML::Key << "MaterialType" << YAML::Value << "unknown_shader";

      out << YAML::Key << "Floats";
      out << YAML::BeginSeq;
      for (auto& uFloat : material->getFloats())
      {
        out << YAML::BeginMap;
        out << YAML::Key << "UniformName" << YAML::Value << uFloat.first;
        out << YAML::Key << "UniformValue" << YAML::Value << uFloat.second;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      out << YAML::Key << "Vec2s";
      out << YAML::BeginSeq;
      for (auto& uVec2 : material->getVec2s())
      {
        out << YAML::BeginMap;
        out << YAML::Key << "UniformName" << YAML::Value << uVec2.first;
        out << YAML::Key << "UniformValue" << YAML::Value << uVec2.second;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      out << YAML::Key << "Vec3s";
      out << YAML::BeginSeq;
      for (auto& uVec3 : material->getVec3s())
      {
        out << YAML::BeginMap;
        out << YAML::Key << "UniformName" << YAML::Value << uVec3.first;
        out << YAML::Key << "UniformValue" << YAML::Value << uVec3.second;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      out << YAML::Key << "Sampler2Ds";
      out << YAML::BeginSeq;
      for (auto& uSampler2D : material->getSampler2Ds())
      {
        out << YAML::BeginMap;
        out << YAML::Key << "SamplerName" << YAML::Value << uSampler2D.first;
        out << YAML::Key << "SamplerHandle" << YAML::Value << uSampler2D.second;
        out << YAML::Key << "ImagePath" << YAML::Value << textureCache->getAsset(uSampler2D.second)->getFilepath();
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      out << YAML::EndMap;
    }

    void serializeMaterial(const AssetHandle &materialHandle,
                           const std::string &filepath)
    {
      YAML::Emitter out;
      serializeMaterial(out, materialHandle);

      std::ofstream output(filepath, std::ofstream::trunc | std::ofstream::out);
      output << out.c_str();
      output.close();
    }

    void
    serializeEntity(YAML::Emitter &out, Entity entity, Shared<Scene> scene)
    {
      if (!entity)
        return;

      auto materialAssets = AssetManager<Material>::getManager();

      // Serialize the entity.
      out << YAML::BeginMap;
      out << YAML::Key << "EntityID" << YAML::Value << (uint) entity;

      if (entity.hasComponent<NameComponent>())
      {
        out << YAML::Key << "NameComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<NameComponent>();
        out << YAML::Key << "Name" << YAML::Value << component.name;
        out << YAML::Key << "Description" << YAML::Value << component.description;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<PrefabComponent>())
      {
        out << YAML::Key << "PrefabComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<PrefabComponent>();
        out << YAML::Key << "Synch" << YAML::Value << component.synch;
        out << YAML::Key << "PreFabID" << YAML::Value << component.prefabID;
        out << YAML::Key << "PreFabPath" << YAML::Value << component.prefabPath;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<ChildEntityComponent>())
      {
        out << YAML::Key << "ChildEntities" << YAML::Value << YAML::BeginSeq;

        auto& children = entity.getComponent<ChildEntityComponent>().children;
        for (auto& child : children)
          serializeEntity(out, child, scene);

        out << YAML::EndSeq;
      }

      if (entity.hasComponent<TransformComponent>())
      {
        out << YAML::Key << "TransformComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<TransformComponent>();
        out << YAML::Key << "Translation" << YAML::Value << component.translation;
        out << YAML::Key << "Rotation" << YAML::Value << component.rotation;
        out << YAML::Key << "Scale" << YAML::Value << component.scale;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<RenderableComponent>())
      {
        auto modelAssets = AssetManager<Model>::getManager();

        auto& component = entity.getComponent<RenderableComponent>();
        auto model = modelAssets->getAsset(component.meshName);
        auto& material = component.materials;

        if (model != nullptr)
        {
          out << YAML::Key << "RenderableComponent";
          out << YAML::BeginMap;

          // Serialize the model path and name.
          out << YAML::Key << "ModelPath" << YAML::Value << model->getFilepath();
          out << YAML::Key << "ModelName" << YAML::Value << component.meshName;

          auto currentAnimation = component.animator.getStoredAnimation();
          if (currentAnimation)
            out << YAML::Key << "CurrentAnimation" << YAML::Value << currentAnimation->getName();
          else
            out << YAML::Key << "CurrentAnimation" << YAML::Value << "None";

          out << YAML::Key << "Material";
          out << YAML::BeginSeq;
          for (auto& pair : material.getStorage())
          {
            auto material = materialAssets->getAsset(pair.first);
            out << YAML::BeginMap;
            out << YAML::Key << "SubmeshName" << YAML::Value << pair.first;
            out << YAML::Key << "MaterialHandle" << YAML::Value << pair.second;
            out << YAML::EndMap;
          }
          out << YAML::EndSeq;

          out << YAML::EndMap;
        }
        else
        {
          out << YAML::Key << "RenderableComponent";
          out << YAML::BeginMap;

          out << YAML::Key << "ModelPath" << YAML::Value << "None";

          out << YAML::EndMap;
        }
      }

      if (entity.hasComponent<CameraComponent>())
      {
        out << YAML::Key << "CameraComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<CameraComponent>();
        out << YAML::Key << "IsPrimary" << YAML::Value << static_cast<bool>(entity == scene->getPrimaryCameraEntity());
        out << YAML::Key << "Near" << YAML::Value << component.entCamera.near;
        out << YAML::Key << "Far" << YAML::Value << component.entCamera.far;
        out << YAML::Key << "FOV" << YAML::Value << component.entCamera.fov;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<SkyAtmosphereComponent>())
      {
        out << YAML::Key << "SkyAtmosphereComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<SkyAtmosphereComponent>();
        out << YAML::Key << "RayleighScatteringFunction" << YAML::Value << component.rayleighScat;
        out << YAML::Key << "RayleighAbsorptionFunction" << YAML::Value << component.rayleighAbs;
        out << YAML::Key << "MieScatteringFunction" << YAML::Value << component.mieScat;
        out << YAML::Key << "MieAbsorptionFunction" << YAML::Value << component.mieAbs;
        out << YAML::Key << "OzoneAbsorptionFunction" << YAML::Value << component.ozoneAbs;
        out << YAML::Key << "PlanetAlbedo" << YAML::Value << component.planetAlbedo;
        out << YAML::Key << "PlanetAndAtmosphereRadius" << YAML::Value << component.planetAtmRadius;
        out << YAML::Key << "UsePrimaryLight" << YAML::Value << component.usePrimaryLight;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<DynamicSkyboxComponent>())
      {
        out << YAML::Key << "DynamicSkyboxComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<DynamicSkyboxComponent>();
        out << YAML::Key << "SunSize" << YAML::Value << component.sunSize;
        out << YAML::Key << "Intensity" << YAML::Value << component.intensity;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<DirectionalLightComponent>())
      {
        out << YAML::Key << "DirectionalLightComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<DirectionalLightComponent>();
        out << YAML::Key << "Direction" << YAML::Value << component.direction;
        out << YAML::Key << "Colour" << YAML::Value << component.colour;
        out << YAML::Key << "Intensity" << YAML::Value << component.intensity;
        out << YAML::Key << "Size" << YAML::Value << component.size;
        out << YAML::Key << "CastShadows" << YAML::Value << component.castShadows;
        out << YAML::Key << "PrimaryLight" << YAML::Value << static_cast<bool>(entity == scene->getPrimaryDirectionalEntity());

        out << YAML::EndMap;
      }

      if (entity.hasComponent<PointLightComponent>())
      {
        out << YAML::Key << "PointLightComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<PointLightComponent>();
        out << YAML::Key << "Colour" << YAML::Value << component.colour;
        out << YAML::Key << "Intensity" << YAML::Value << component.intensity;
        out << YAML::Key << "Radius" << YAML::Value << component.radius;
        out << YAML::Key << "CastShadows" << YAML::Value << component.castShadows;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<DynamicSkylightComponent>())
      {
        out << YAML::Key << "DynamicSkylightComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<DynamicSkylightComponent>();
        out << YAML::Key << "Intensity" << YAML::Value << component.intensity;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<SphereColliderComponent>())
      {
        out << YAML::Key << "SphereColliderComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<SphereColliderComponent>();
        out << YAML::Key << "Radius" << YAML::Value << component.radius;
        out << YAML::Key << "Offset" << YAML::Value << component.offset;
        out << YAML::Key << "Density" << YAML::Value << component.density;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<BoxColliderComponent>())
      {
        out << YAML::Key << "BoxColliderComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<BoxColliderComponent>();
        out << YAML::Key << "HalfExtents" << YAML::Value << component.extents;
        out << YAML::Key << "Offset" << YAML::Value << component.offset;
        out << YAML::Key << "Density" << YAML::Value << component.density;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<CylinderColliderComponent>())
      {
        out << YAML::Key << "CylinderColliderComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<CylinderColliderComponent>();
        out << YAML::Key << "HalfHeight" << YAML::Value << component.halfHeight;
        out << YAML::Key << "Radius" << YAML::Value << component.radius;
        out << YAML::Key << "Offset" << YAML::Value << component.offset;
        out << YAML::Key << "Density" << YAML::Value << component.density;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<CapsuleColliderComponent>())
      {
        out << YAML::Key << "CapsuleColliderComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<CapsuleColliderComponent>();
        out << YAML::Key << "HalfHeight" << YAML::Value << component.halfHeight;
        out << YAML::Key << "Radius" << YAML::Value << component.radius;
        out << YAML::Key << "Offset" << YAML::Value << component.offset;
        out << YAML::Key << "Density" << YAML::Value << component.density;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<RigidBody3DComponent>())
      {
        out << YAML::Key << "RigidBody3DComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<RigidBody3DComponent>();
        out << YAML::Key << "Type" << YAML::Value << static_cast<uint>(component.type);
        out << YAML::Key << "Friction" << YAML::Value << component.friction;
        out << YAML::Key << "Restitution" << YAML::Value << component.restitution;

        out << YAML::EndMap;
      }

      out << YAML::EndMap;
    }

    void
    serializeScene(Shared<Scene> scene, const std::string &filepath,
                   const std::string &name)
    {
      auto materialAssets = AssetManager<Material>::getManager();

      YAML::Emitter out;
      out << YAML::BeginMap;
      out << YAML::Key << "Scene" << YAML::Value << name;
      out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

      scene->getRegistry().each([&](auto entityID)
      {
        Entity entity(entityID, scene.get());
        if (!entity)
          return;

        if (entity.hasComponent<ParentEntityComponent>())
          return;

        serializeEntity(out, entity, scene);
      });

      out << YAML::EndSeq;

      // Serialize renderer settings.
      out << YAML::Key << "RendererSettings" << YAML::BeginMap;
      {
        auto& rendererStorage = Renderer3D::getStorage();
        auto& passManager = Renderer3D::getPassManager();

        // General renderer settings.
        {
          out << YAML::Key << "GeneralSettings" << YAML::BeginMap;

          out << YAML::Key << "Gamma" << YAML::Value << rendererStorage.gamma;

          out << YAML::EndMap;
        }

        // Shadow settings.
        {
          auto shadowPass = passManager.getRenderPass<ShadowPass>();
          auto shadowPassData = shadowPass->getInternalDataBlock<ShadowPassDataBlock>();

          out << YAML::Key << "ShadowSettings" << YAML::BeginMap;

          out << YAML::Key << "ShadowQuality" << YAML::Value << shadowPassData->shadowQuality;
          out << YAML::Key << "MinimumRadius" << YAML::Value << shadowPassData->minRadius;
          out << YAML::Key << "CascadeLambda" << YAML::Value << shadowPassData->cascadeLambda;
          out << YAML::Key << "ConstantBias" << YAML::Value << shadowPassData->constBias;
          out << YAML::Key << "NormalBias" << YAML::Value << shadowPassData->normalBias;
          out << YAML::Key << "ShadowMapResolution" << YAML::Value << shadowPassData->shadowMapRes;

          out << YAML::EndMap;
        }

        // AO settings.
        {
          auto aoPass = passManager.getRenderPass<HBAOPass>();
          auto aoPassData = aoPass->getInternalDataBlock<HBAOPassDataBlock>();

          out << YAML::Key << "HBAOSettings" << YAML::BeginMap;

          out << YAML::Key << "UseHBAO" << YAML::Value << aoPassData->enableAO;
          out << YAML::Key << "AORadius" << YAML::Value << aoPassData->aoRadius;
          out << YAML::Key << "AOMultiplier" << YAML::Value << aoPassData->aoMultiplier;
          out << YAML::Key << "AOExponent" << YAML::Value << aoPassData->aoExponent;

          out << YAML::EndMap;
        }

        // Sky-atmosphere settings.
        {
          auto skyAtmoPass = passManager.getRenderPass<SkyAtmospherePass>();
          auto skyAtmoPassData = skyAtmoPass->getInternalDataBlock<SkyAtmospherePassDataBlock>();

          out << YAML::Key << "SkyAtmosphereSettings" << YAML::BeginMap;

          out << YAML::Key << "UseFastAtmosphere" << YAML::Value << skyAtmoPassData->useFastAtmosphere;

          out << YAML::EndMap;
        }

        // Dynamic sky IBL settings.
        {
          auto dynSkyIBLPass = passManager.getRenderPass<DynamicSkyIBLPass>();
          auto dynSkyIBLPassData = dynSkyIBLPass->getInternalDataBlock<DynamicSkyIBLPassDataBlock>();

          out << YAML::Key << "DynamicSkyIBLSettings" << YAML::BeginMap;

          out << YAML::Key << "IrradianceSamples" << YAML::Value << dynSkyIBLPassData->numIrradSamples;
          out << YAML::Key << "RadianceSamples" << YAML::Value << dynSkyIBLPassData->numRadSamples;

          out << YAML::EndMap;
        }

        // Bloom settings
        {
          auto bloomPass = passManager.getRenderPass<BloomPass>();
          auto bloomPassData = bloomPass->getInternalDataBlock<BloomPassDataBlock>();

          out << YAML::Key << "BloomSettings" << YAML::BeginMap;
          
          out << YAML::Key << "UseBloom" << YAML::Value << bloomPassData->useBloom;
          out << YAML::Key << "Threshold" << YAML::Value << bloomPassData->threshold;
          out << YAML::Key << "Knee" << YAML::Value << bloomPassData->knee;
          out << YAML::Key << "Radius" << YAML::Value << bloomPassData->radius;
          out << YAML::Key << "Intensity" << YAML::Value << bloomPassData->intensity;

          out << YAML::EndMap;
        }

        // General post-processing settings.
        {
          auto postPass = passManager.getRenderPass<PostProcessingPass>();
          auto postPassData = postPass->getInternalDataBlock<PostProcessingPassDataBlock>();

          out << YAML::Key << "GeneralPostProcessingSettings" << YAML::BeginMap;

          out << YAML::Key << "UseFXAA" << YAML::Value << postPassData->useFXAA;
          out << YAML::Key << "ToneMapFunction" << YAML::Value << postPassData->toneMapOp;

          out << YAML::EndMap;
        }
      }
      out << YAML::EndMap;

      out << YAML::Key << "Materials";
      out << YAML::BeginSeq;
      for (auto& materialHandle : materialAssets->getStorage())
        serializeMaterial(out, materialHandle, true);
      out << YAML::EndSeq;

      out << YAML::EndMap;

      std::ofstream output(filepath, std::ofstream::trunc | std::ofstream::out);
      output << out.c_str();
      output.close();
    }

    void
    serializePrefab(Entity prefab, const std::string &filepath,
                    Shared<Scene> scene, const std::string &name)
    {
      YAML::Emitter out;
      out << YAML::BeginMap;
      out << YAML::Key << "PreFab" << YAML::Value << name;
      out << YAML::Key << "EntityInfo";

      serializeEntity(out, prefab, scene);

      out << YAML::EndMap;

      std::ofstream output(filepath, std::ofstream::trunc | std::ofstream::out);
      output << out.c_str();
      output.close();
    }

    void
    deserializeMaterial(YAML::Node &mat, std::vector<std::string> &texturePaths,
                        bool override = false, const std::string &filepath = "")
    {
      auto materialAssets = AssetManager<Material>::getManager();

      auto parsedMaterialName = mat["MaterialName"];
      auto parsedMaterialPath = mat["MaterialPath"];

      std::string materialPath = "";
      if (parsedMaterialPath && override)
      {
        materialPath = parsedMaterialPath.as<std::string>();
        AssetHandle handle;
        deserializeMaterial(materialPath, handle);
        return;
      }
      else if (!parsedMaterialPath && filepath != "")
        materialPath = filepath;

      auto matType = mat["MaterialType"];

      std::string shaderName, materialName;
      if (matType && parsedMaterialName)
      {
        Material* outMat;

        shaderName = matType.as<std::string>();
        materialName = parsedMaterialName.as<std::string>();
        if (shaderName == "pbr_shader")
          outMat = new Material(MaterialType::PBR);
        else
          outMat = new Material(MaterialType::Unknown);

        materialAssets->attachAsset(materialName, outMat);

        outMat->getFilepath() = materialPath;

        auto floats = mat["Floats"];
        if (floats)
        {
          for (auto uFloat : floats)
          {
            auto uName = uFloat["UniformName"];
            if (uName)
              outMat->set(uFloat["UniformValue"].as<float>(), uName.as<std::string>());
          }
        }

        auto vec2s = mat["Vec2s"];
        if (vec2s)
        {
          for (auto uVec2 : vec2s)
          {
            auto uName = uVec2["UniformName"];
            if (uName)
              outMat->set(uVec2["UniformValue"].as<glm::vec2>(), uName.as<std::string>());
          }
        }

        auto vec3s = mat["Vec3s"];
        if (vec3s)
        {
          for (auto uVec3 : vec3s)
          {
            auto uName = uVec3["UniformName"];
            if (uName)
              outMat->set(uVec3["UniformValue"].as<glm::vec3>(), uName.as<std::string>());
          }
        }

        auto sampler2Ds = mat["Sampler2Ds"];
        if (sampler2Ds)
        {
          auto textureCache = AssetManager<Texture2D>::getManager();

          for (auto uSampler2D : sampler2Ds)
          {
            auto uName = uSampler2D["SamplerName"];
            if (uName)
            {
              if (uSampler2D["ImagePath"].as<std::string>() == "")
                continue;

              bool shouldLoad = std::find(texturePaths.begin(),
                                          texturePaths.end(),
                                          uSampler2D["ImagePath"].as<std::string>()
                                          ) == texturePaths.end();

              if (!textureCache->hasAsset(uSampler2D["SamplerHandle"].as<std::string>()) && shouldLoad)
                texturePaths.emplace_back(uSampler2D["ImagePath"].as<std::string>());

              outMat->attachSampler2D(uSampler2D["SamplerName"].as<std::string>(),
                                      uSampler2D["SamplerHandle"].as<std::string>());
            }
          }
        }
      }
    }

    bool deserializeMaterial(const std::string &filepath, AssetHandle &handle, bool override)
    {
      YAML::Node data = YAML::LoadFile(filepath);

      if (!data["MaterialName"])
        return false;

      handle = data["MaterialName"].as<std::string>();

      std::vector<std::string> texturePaths;
      deserializeMaterial(data, texturePaths, false, filepath);

      for (auto& texturePath : texturePaths)
        AsyncLoading::loadImageAsync(texturePath);

      return true;
    }

    Entity
    deserializeEntity(YAML::Node &entity, Shared<Scene> scene, Entity parent = Entity())
    {
      uint entityID = entity["EntityID"].as<uint>();

      Entity newEntity = scene->createEntity(entityID);

      auto nameComponent = entity["NameComponent"];
      if (nameComponent)
      {
        std::string name = nameComponent["Name"].as<std::string>();
        std::string description = nameComponent["Description"].as<std::string>();

        auto& nComponent = newEntity.getComponent<NameComponent>();
        nComponent.name = name;
        nComponent.description = description;
      }

      auto prefabComponent = entity["PrefabComponent"];
      if (prefabComponent)
      {
        auto pfID = prefabComponent["PreFabID"].as<std::string>();
        auto pfPath = prefabComponent["PreFabPath"].as<std::string>();
        auto& pfComponent = newEntity.addComponent<PrefabComponent>(pfID, pfPath);
        pfComponent.synch = prefabComponent["Synch"].as<bool>();
      }

      auto childEntityComponents = entity["ChildEntities"];
      if (childEntityComponents)
      {
        auto& children = newEntity.addComponent<ChildEntityComponent>().children;

        for (auto childNode : childEntityComponents)
        {
          Entity child = deserializeEntity(childNode, scene, newEntity);
          children.push_back(child);
        }
      }

      if (parent)
        newEntity.addComponent<ParentEntityComponent>(parent);

      auto transformComponent = entity["TransformComponent"];
      if (transformComponent)
      {
        glm::vec3 translation = transformComponent["Translation"].as<glm::vec3>();
        glm::vec3 rotation = transformComponent["Rotation"].as<glm::vec3>();
        glm::vec3 scale = transformComponent["Scale"].as<glm::vec3>();

        newEntity.addComponent<TransformComponent>(translation, rotation, scale);
      }

      auto renderableComponent = entity["RenderableComponent"];
      if (renderableComponent)
      {
        std::string modelPath = renderableComponent["ModelPath"].as<std::string>();

        std::ifstream test(modelPath);
        if (test)
        {
          std::string modelName = renderableComponent["ModelName"].as<std::string>();
          auto& rComponent = newEntity.addComponent<RenderableComponent>(modelName);

          // If the path is "None" its an internal model asset.
          // TODO: Handle internals separately.
          if (modelPath != "None")
          {
            auto animationName = renderableComponent["CurrentAnimation"];
            if (animationName)
              rComponent.animationHandle = animationName.as<std::string>();

            auto materials = renderableComponent["Material"];
            if (materials)
            {
              for (auto mat : materials)
              {
                rComponent.materials.attachMesh(mat["SubmeshName"].as<std::string>(),
                                                mat["MaterialHandle"].as<std::string>());
              }
            }
            AsyncLoading::asyncLoadModel(modelPath, modelName, newEntity, scene.get());
          }
        }
        else
          Logs::log("Error, file " + modelPath + " cannot be opened.");
      }

      auto camComponent = entity["CameraComponent"];
      if (camComponent)
      {
        auto& camera = newEntity.addComponent<CameraComponent>();
        camera.entCamera.near = camComponent["Near"].as<float>();
        camera.entCamera.far = camComponent["Far"].as<float>();
        camera.entCamera.fov = camComponent["FOV"].as<float>();
        if (camComponent["IsPrimary"].as<bool>())
          scene->setPrimaryCameraEntity(newEntity);
      }

      auto skyAtmosphereComponent = entity["SkyAtmosphereComponent"];
      if (skyAtmosphereComponent)
      {
        auto& saComponent = newEntity.addComponent<SkyAtmosphereComponent>();
        saComponent.rayleighScat = skyAtmosphereComponent["RayleighScatteringFunction"].as<glm::vec4>();
        saComponent.rayleighAbs = skyAtmosphereComponent["RayleighAbsorptionFunction"].as<glm::vec4>();
        saComponent.mieScat = skyAtmosphereComponent["MieScatteringFunction"].as<glm::vec4>();
        saComponent.mieAbs = skyAtmosphereComponent["MieAbsorptionFunction"].as<glm::vec4>();
        saComponent.ozoneAbs = skyAtmosphereComponent["OzoneAbsorptionFunction"].as<glm::vec4>();
        saComponent.planetAlbedo = skyAtmosphereComponent["PlanetAlbedo"].as<glm::vec3>();
        saComponent.planetAtmRadius = skyAtmosphereComponent["PlanetAndAtmosphereRadius"].as<glm::vec2>();
        saComponent.usePrimaryLight = skyAtmosphereComponent["UsePrimaryLight"].as<bool>();
      }

      auto dynamicSkyboxComponent = entity["DynamicSkyboxComponent"];
      if (dynamicSkyboxComponent)
      {
        auto& dsComponent = newEntity.addComponent<DynamicSkyboxComponent>();
        dsComponent.sunSize = dynamicSkyboxComponent["SunSize"].as<float>();
        dsComponent.intensity = dynamicSkyboxComponent["Intensity"].as<float>();
      }
      
      auto directionalComponent = entity["DirectionalLightComponent"];
      if (directionalComponent)
      {
        auto& dComponent = newEntity.addComponent<DirectionalLightComponent>();
        dComponent.direction = directionalComponent["Direction"].as<glm::vec3>();
        dComponent.colour = directionalComponent["Colour"].as<glm::vec3>();
        dComponent.intensity = directionalComponent["Intensity"].as<float>();
        dComponent.size = directionalComponent["Size"].as<float>();
        dComponent.castShadows = directionalComponent["CastShadows"].as<bool>();
        if (directionalComponent["PrimaryLight"])
          scene->setPrimaryDirectionalEntity(newEntity);
      }

      auto pointComponent = entity["PointLightComponent"];
      if (pointComponent)
      {
        auto& pComponent = newEntity.addComponent<PointLightComponent>();
        pComponent.colour = pointComponent["Colour"].as<glm::vec3>();
        pComponent.intensity = pointComponent["Intensity"].as<float>();
        pComponent.radius = pointComponent["Radius"].as<float>();
        pComponent.castShadows = pointComponent["CastShadows"].as<bool>();
      }

      auto dynamicSkyLightComponent = entity["DynamicSkylightComponent"];
      if (dynamicSkyLightComponent)
      {
        auto& dsComponent = newEntity.addComponent<DynamicSkylightComponent>();
        dsComponent.intensity = dynamicSkyLightComponent["Intensity"].as<float>();
      }

      auto sphereColliderComponent = entity["SphereColliderComponent"];
      if (sphereColliderComponent)
      {
        auto& scComponent = newEntity.addComponent<SphereColliderComponent>();
        scComponent.radius = sphereColliderComponent["Radius"].as<float>();
        scComponent.offset = sphereColliderComponent["Offset"].as<glm::vec3>();
        scComponent.density = sphereColliderComponent["Density"].as<float>();
      }

      auto boxColliderComponent = entity["BoxColliderComponent"];
      if (boxColliderComponent)
      {
        auto& bcComponent = newEntity.addComponent<BoxColliderComponent>();
        bcComponent.extents = boxColliderComponent["HalfExtents"].as<glm::vec3>();
        bcComponent.offset = boxColliderComponent["Offset"].as<glm::vec3>();
        bcComponent.density = boxColliderComponent["Density"].as<float>();
      }

      auto cylinderColliderComponent = entity["CylinderColliderComponent"];
      if (cylinderColliderComponent)
      {
        auto& cComponent = newEntity.addComponent<CylinderColliderComponent>();
        cComponent.halfHeight = cylinderColliderComponent["HalfHeight"].as<float>();
        cComponent.radius = cylinderColliderComponent["Radius"].as<float>();
        cComponent.offset = cylinderColliderComponent["Offset"].as<glm::vec3>();
        cComponent.density = cylinderColliderComponent["Density"].as<float>();
      }

      auto capsuleColliderComponent = entity["CapsuleColliderComponent"];
      if (capsuleColliderComponent)
      {
        auto& cComponent = newEntity.addComponent<CapsuleColliderComponent>();
        cComponent.halfHeight = capsuleColliderComponent["HalfHeight"].as<float>();
        cComponent.radius = capsuleColliderComponent["Radius"].as<float>();
        cComponent.offset = capsuleColliderComponent["Offset"].as<glm::vec3>();
        cComponent.density = capsuleColliderComponent["Density"].as<float>();
      }

      auto rigidBody3DComponent = entity["RigidBody3DComponent"];
      if (rigidBody3DComponent)
      {
        auto& rb3DComponent = newEntity.addComponent<RigidBody3DComponent>();
        rb3DComponent.type = static_cast<PhysicsEngine::RigidBodyTypes>(rigidBody3DComponent["Type"].as<uint>());
        rb3DComponent.friction = rigidBody3DComponent["Friction"].as<float>();
        rb3DComponent.restitution = rigidBody3DComponent["Restitution"].as<float>();
      }

      return newEntity;
    }

    bool
    deserializeScene(Shared<Scene> scene, const std::string &filepath)
    {
      YAML::Node data = YAML::LoadFile(filepath);

      if (!data["Scene"])
        return false;

      auto entities = data["Entities"];
      if (!entities)
        return false;

      for (auto entity : entities)
        deserializeEntity(entity, scene);

      auto rendererSettings = data["RendererSettings"];
      if (rendererSettings)
      {
        auto& rendererStorage = Renderer3D::getStorage();
        auto& passManager = Renderer3D::getPassManager();

        {
          auto generalRendererSettings = rendererSettings["GeneralSettings"];
          if (generalRendererSettings)
          {
            rendererStorage.gamma = generalRendererSettings["Gamma"].as<float>();
          }
        }

        {
          auto shadowSettings = rendererSettings["ShadowSettings"];
          if (shadowSettings)
          {
            auto shadowPass = passManager.getRenderPass<ShadowPass>();
            auto shadowPassData = shadowPass->getInternalDataBlock<ShadowPassDataBlock>();

            shadowPassData->shadowQuality = shadowSettings["ShadowQuality"].as<uint>();
            shadowPassData->minRadius = shadowSettings["MinimumRadius"].as<float>();
            shadowPassData->cascadeLambda = shadowSettings["CascadeLambda"].as<float>();
            shadowPassData->constBias = shadowSettings["ConstantBias"].as<float>();
            shadowPassData->normalBias = shadowSettings["NormalBias"].as<float>();
            shadowPassData->shadowMapRes = shadowSettings["ShadowMapResolution"].as<uint>();
            shadowPass->updatePassData();
          }
        }

        {
          auto hbaoSettings = rendererSettings["HBAOSettings"];
          if (hbaoSettings)
          {
            auto aoPass = passManager.getRenderPass<HBAOPass>();
            auto aoPassData = aoPass->getInternalDataBlock<HBAOPassDataBlock>();

            aoPassData->enableAO = hbaoSettings["UseHBAO"].as<bool>();
            aoPassData->aoRadius = hbaoSettings["AORadius"].as<float>();
            aoPassData->aoMultiplier = hbaoSettings["AOMultiplier"].as<float>();
            aoPassData->aoExponent = hbaoSettings["AOExponent"].as<float>();
          }
        }

        {
          auto skyAtmoSettings = rendererSettings["SkyAtmosphereSettings"];
          if (skyAtmoSettings)
          {
            auto skyAtmoPass = passManager.getRenderPass<SkyAtmospherePass>();
            auto skyAtmoPassData = skyAtmoPass->getInternalDataBlock<SkyAtmospherePassDataBlock>();

            skyAtmoPassData->useFastAtmosphere = skyAtmoSettings["UseFastAtmosphere"].as<bool>();
          }
        }

        {
          auto dynSkyIBLSettings = rendererSettings["DynamicSkyIBLSettings"];
          if (dynSkyIBLSettings)
          {
            auto dynSkyIBLPass = passManager.getRenderPass<DynamicSkyIBLPass>();
            auto dynSkyIBLPassData = dynSkyIBLPass->getInternalDataBlock<DynamicSkyIBLPassDataBlock>();

            dynSkyIBLPassData->numIrradSamples = dynSkyIBLSettings["IrradianceSamples"].as<uint>();
            dynSkyIBLPassData->numRadSamples = dynSkyIBLSettings["RadianceSamples"].as<uint>();
          }
        }

        {
          auto bloomSettings = rendererSettings["BloomSettings"];
          if (bloomSettings)
          {
            auto bloomPass = passManager.getRenderPass<BloomPass>();
            auto bloomPassData = bloomPass->getInternalDataBlock<BloomPassDataBlock>();

            bloomPassData->useBloom = bloomSettings["UseBloom"].as<bool>();
            bloomPassData->threshold = bloomSettings["Threshold"].as<float>();
            bloomPassData->knee = bloomSettings["Knee"].as<float>();
            bloomPassData->radius = bloomSettings["Radius"].as<float>();
            bloomPassData->intensity = bloomSettings["Intensity"].as<float>();
          }
        }

        {
          auto generalPostSettings = rendererSettings["GeneralPostProcessingSettings"];
          if (generalPostSettings)
          {
            auto postPass = passManager.getRenderPass<PostProcessingPass>();
            auto postPassData = postPass->getInternalDataBlock<PostProcessingPassDataBlock>();

            postPassData->useFXAA = generalPostSettings["UseFXAA"].as<bool>();
            postPassData->toneMapOp = generalPostSettings["ToneMapFunction"].as<uint>();
          }
        }
      }

      auto materials = data["Materials"];
      if (materials)
      {
        std::vector<std::string> texturePaths;
        for (auto mat : materials)
          deserializeMaterial(mat, texturePaths, true);

        for (auto& texturePath : texturePaths)
          AsyncLoading::loadImageAsync(texturePath);
      }
      

      return true;
    }

    bool
    deserializePrefab(Shared<Scene> scene, const std::string &filepath)
    {
      YAML::Node data = YAML::LoadFile(filepath);

      if (!data["PreFab"])
        return false;

      auto preFabName = data["PreFab"].as<std::string>();

      auto preFabInfo = data["EntityInfo"];
      if (data["EntityInfo"])
      {
        auto preFabEntity = deserializeEntity(preFabInfo, scene);

        return true;
      }
      else
        return false;
    }
  }
}
