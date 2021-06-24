#include "Serialization/YamlSerialization.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Scenes/Components.h"
#include "Scenes/Entity.h"
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

namespace SciRenderer
{
  namespace YAMLSerialization
  {
    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
    {
    	out << YAML::Flow;
    	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    	return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
    	out << YAML::Flow;
    	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    	return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
    	out << YAML::Flow;
    	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    	return out;
    }

    void serializeMaterial(YAML::Emitter &out, std::pair<std::string, Material> &pair)
    {
      auto textureCache = AssetManager<Texture2D>::getManager();

      out << YAML::BeginMap;

      // Save the material type.
      if (pair.second.getType() == MaterialType::PBR)
        out << YAML::Key << "MaterialType" << YAML::Value << "pbr_shader";
      else if (pair.second.getType() == MaterialType::Specular)
        out << YAML::Key << "MaterialType" << YAML::Value << "specular_shader";
      else
        out << YAML::Key << "MaterialType" << YAML::Value << "unknown_shader";

      // Save the submesh which this acts on.
      out << YAML::Key << "AssociatedSubmesh" << YAML::Value << pair.first;

      out << YAML::Key << "Floats";
      out << YAML::BeginSeq;
      for (auto& uFloat : pair.second.getFloats())
      {
        out << YAML::BeginMap;
        out << YAML::Key << "UniformName" << YAML::Value << uFloat.first;
        out << YAML::Key << "UniformValue" << YAML::Value << uFloat.second;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      out << YAML::Key << "Vec2s";
      out << YAML::BeginSeq;
      for (auto& uVec2 : pair.second.getVec2s())
      {
        out << YAML::BeginMap;
        out << YAML::Key << "UniformName" << YAML::Value << uVec2.first;
        out << YAML::Key << "UniformValue" << YAML::Value << uVec2.second;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      out << YAML::Key << "Vec3s";
      out << YAML::BeginSeq;
      for (auto& uVec3 : pair.second.getVec3s())
      {
        out << YAML::BeginMap;
        out << YAML::Key << "UniformName" << YAML::Value << uVec3.first;
        out << YAML::Key << "UniformValue" << YAML::Value << uVec3.second;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      out << YAML::Key << "Sampler2Ds";
      out << YAML::BeginSeq;
      for (auto& uSampler2D : pair.second.getSampler2Ds())
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

    void serializeEntity(YAML::Emitter &out, Entity entity)
    {
      if (!entity)
        return;

      // Serialize the entity.
      out << YAML::BeginMap;
      out << YAML::Key << "EntityID" << YAML::Value << (GLuint) entity;

      if (entity.hasComponent<NameComponent>())
      {
        out << YAML::Key << "NameComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<NameComponent>();
        out << YAML::Key << "Name" << YAML::Value << component.name;
        out << YAML::Key << "Description" << YAML::Value << component.description;

        out << YAML::EndMap;
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

          // TODO: Serialize materials.
          out << YAML::Key << "Material";
          out << YAML::BeginSeq;
          for (auto& pair : material.getStorage())
          {
            serializeMaterial(out, pair);
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

      if (entity.hasComponent<DirectionalLightComponent>())
      {
        out << YAML::Key << "DirectionalLightComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<DirectionalLightComponent>();
        out << YAML::Key << "Direction" << YAML::Value << component.light.direction;
        out << YAML::Key << "Colour" << YAML::Value << component.light.colour;
        out << YAML::Key << "Intensity" << YAML::Value << component.light.intensity;
        out << YAML::Key << "CastShadows" << YAML::Value << component.light.castShadows;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<PointLightComponent>())
      {
        out << YAML::Key << "PointLightComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<PointLightComponent>();
        out << YAML::Key << "Position" << YAML::Value << component.light.position;
        out << YAML::Key << "Colour" << YAML::Value << component.light.colour;
        out << YAML::Key << "Intensity" << YAML::Value << component.light.intensity;
        out << YAML::Key << "Radius" << YAML::Value << component.light.radius;
        out << YAML::Key << "CastShadows" << YAML::Value << component.light.castShadows;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<SpotLightComponent>())
      {
        out << YAML::Key << "SpotLightComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<SpotLightComponent>();
        out << YAML::Key << "Position" << YAML::Value << component.light.position;
        out << YAML::Key << "Direction" << YAML::Value << component.light.direction;
        out << YAML::Key << "Colour" << YAML::Value << component.light.colour;
        out << YAML::Key << "Intensity" << YAML::Value << component.light.intensity;
        out << YAML::Key << "InnerCutoff" << YAML::Value << component.light.innerCutoff;
        out << YAML::Key << "OuterCutoff" << YAML::Value << component.light.outerCutoff;
        out << YAML::Key << "Radius" << YAML::Value << component.light.radius;
        out << YAML::Key << "CastShadows" << YAML::Value << component.light.castShadows;

        out << YAML::EndMap;
      }

      if (entity.hasComponent<AmbientComponent>())
      {
        out << YAML::Key << "AmbientComponent";
        out << YAML::BeginMap;

        auto& component = entity.getComponent<AmbientComponent>();
        auto state = Renderer3D::getState();
        out << YAML::Key << "IBLPath" << YAML::Value << component.ambient->getFilepath();
        out << YAML::Key << "EnviRes" << YAML::Value << state->skyboxWidth;
        out << YAML::Key << "IrraRes" << YAML::Value << state->irradianceWidth;
        out << YAML::Key << "FiltRes" << YAML::Value << state->prefilterWidth;
        out << YAML::Key << "FiltSam" << YAML::Value << state->prefilterSamples;
        out << YAML::Key << "IBLGamma" << YAML::Value << component.ambient->getGamma();
        out << YAML::Key << "IBLRough" << YAML::Value << component.ambient->getRoughness();

        out << YAML::EndMap;
      }

      out << YAML::EndMap;
    }

    void serializeScene(Shared<Scene> scene, const std::string &filepath,
                        const std::string &name)
    {
      YAML::Emitter out;
      out << YAML::BeginMap;
      out << YAML::Key << "Scene" << YAML::Value << name;
      out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

      scene->getRegistry().each([&](auto entityID)
      {
        Entity entity(entityID, scene.get());
        if (!entity)
          return;

        serializeEntity(out, entity);
      });

      out << YAML::EndSeq;
      out << YAML::EndMap;

      std::ofstream output(filepath, std::ofstream::trunc | std::ofstream::out);
      output << out.c_str();
      output.close();
    }

    void serializeSettings()
    {

    }

    void deserializeMaterial(YAML::Node &mat, Entity entity)
    {
      auto matType = mat["MaterialType"];
      auto parsedSubmeshName = mat["AssociatedSubmesh"];
      auto& modelMaterial = entity.getComponent<RenderableComponent>().materials;

      std::string shaderName, submeshName;
      if (matType && parsedSubmeshName)
      {
        shaderName = matType.as<std::string>();
        submeshName = parsedSubmeshName.as<std::string>();
        if (shaderName == "pbr_shader")
          modelMaterial.attachMesh(submeshName, MaterialType::PBR);
        else if (shaderName == "specular_shader")
          modelMaterial.attachMesh(submeshName, MaterialType::Specular);
        else
          modelMaterial.attachMesh(submeshName, MaterialType::Unknown);

        auto meshMaterial = modelMaterial.getMaterial(submeshName);

        auto floats = mat["Floats"];
        if (floats)
        {
          for (auto uFloat : floats)
          {
            auto uName = uFloat["UniformName"];
            if (uName)
              meshMaterial->getFloat(uName.as<std::string>()) = uFloat["UniformValue"].as<GLfloat>();
          }
        }

        auto vec2s = mat["Vec2s"];
        if (vec2s)
        {
          for (auto uVec2 : vec2s)
          {
            auto uName = uVec2["UniformName"];
            if (uName)
              meshMaterial->getVec2(uName.as<std::string>()) = uVec2["UniformValue"].as<glm::vec2>();
          }
        }

        auto vec3s = mat["Vec3s"];
        if (vec3s)
        {
          for (auto uVec3 : vec3s)
          {
            auto uName = uVec3["UniformName"];
            if (uName)
              meshMaterial->getVec3(uName.as<std::string>()) = uVec3["UniformValue"].as<glm::vec3>();
          }
        }

        auto sampler2Ds = mat["Sampler2Ds"];
        if (sampler2Ds)
        {
          for (auto uSampler2D : sampler2Ds)
          {
            auto uName = uSampler2D["SamplerName"];
            if (uName)
            {
              if (uSampler2D["ImagePath"].as<std::string>() == "")
                continue;

              Texture2D::loadImageAsync(uSampler2D["ImagePath"].as<std::string>());
              meshMaterial->attachSampler2D(uSampler2D["SamplerName"].as<std::string>(),
                                            uSampler2D["SamplerHandle"].as<std::string>());
            }
          }
        }
      }
    }

    bool deserializeScene(Shared<Scene> scene, const std::string &filepath)
    {
      YAML::Node data = YAML::LoadFile(filepath);

      if (!data["Scene"])
        return false;

      auto entities = data["Entities"];
      if (!entities)
        return false;

      for (auto entity : entities)
      {
        GLuint entityID = entity["EntityID"].as<GLuint>();

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
          std::string modelName = renderableComponent["ModelName"].as<std::string>();
          auto& rComponent = newEntity.addComponent<RenderableComponent>(modelName);

          // If the path is "None" its an internal model asset.
          // TODO: Handle internals separately.
          if (modelPath != "None")
          {
            auto materials = renderableComponent["Material"];
            if (materials)
              for (auto mat : materials)
                deserializeMaterial(mat, newEntity);
            Model::asyncLoadModel(modelPath, modelName, &rComponent.materials);
          }
        }

        auto directionalComponent = entity["DirectionalLightComponent"];
        if (directionalComponent)
        {
          auto& dComponent = newEntity.addComponent<DirectionalLightComponent>();
          dComponent.light.direction = directionalComponent["Direction"].as<glm::vec3>();
          dComponent.light.colour = directionalComponent["Colour"].as<glm::vec3>();
          dComponent.light.intensity = directionalComponent["Intensity"].as<GLfloat>();
          dComponent.light.castShadows = directionalComponent["CastShadows"].as<bool>();
        }

        auto pointComponent = entity["PointLightComponent"];
        if (pointComponent)
        {
          auto& pComponent = newEntity.addComponent<PointLightComponent>();
          pComponent.light.position = pointComponent["Position"].as<glm::vec3>();
          pComponent.light.colour = pointComponent["Colour"].as<glm::vec3>();
          pComponent.light.intensity = pointComponent["Intensity"].as<GLfloat>();
          pComponent.light.radius = pointComponent["Radius"].as<GLfloat>();
          pComponent.light.castShadows = pointComponent["CastShadows"].as<bool>();
        }

        auto spotComponent = entity["SpotLightComponent"];
        if (spotComponent)
        {
          auto& sComponent = newEntity.addComponent<SpotLightComponent>();
          sComponent.light.position = spotComponent["Position"].as<glm::vec3>();
          sComponent.light.direction = spotComponent["Direction"].as<glm::vec3>();
          sComponent.light.colour = spotComponent["Colour"].as<glm::vec3>();
          sComponent.light.intensity = spotComponent["Intensity"].as<GLfloat>();
          sComponent.light.innerCutoff = spotComponent["InnerCutoff"].as<GLfloat>();
          sComponent.light.outerCutoff = spotComponent["OuterCutoff"].as<GLfloat>();
          sComponent.light.radius = spotComponent["Radius"].as<GLfloat>();
          sComponent.light.castShadows = spotComponent["CastShadows"].as<bool>();
        }

        auto ambientComponent = entity["AmbientComponent"];
        if (ambientComponent)
        {
          auto state = Renderer3D::getState();
          auto storage = Renderer3D::getStorage();

          std::string iblImagePath = ambientComponent["IBLPath"].as<std::string>();
          state->skyboxWidth = ambientComponent["EnviRes"].as<GLuint>();
          state->irradianceWidth = ambientComponent["IrraRes"].as<GLuint>();
          state->prefilterWidth = ambientComponent["FiltRes"].as<GLuint>();
          state->prefilterSamples = ambientComponent["FiltSam"].as<GLuint>();
          storage->currentEnvironment->unloadEnvironment();

          newEntity.addComponent<AmbientComponent>(iblImagePath);
        }
      }

      return true;
    }

    bool deserializeSettings()
    {

    }
  }
}
