// Maximum bones which can influence a vertex.
#define MAX_BONES_PER_VERTEX 4
// Maximum bones in a single model file. Using SSBOs to pass them to the skinning shaders.
#define MAX_BONES_PER_MODEL 512

#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Assimp includes.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Strontium
{
  class Model;

  struct VertexBone
  {
    std::string name;
    std::string parentMesh;

    glm::mat4 offsetMatrix;

    VertexBone(const std::string &name, const std::string &parentMesh,
               const glm::mat4 &offsetMatrix)
      : name(name)
      , parentMesh(parentMesh)
      , offsetMatrix(offsetMatrix)
    { }
  };

  struct AnimationNode
  {
    std::string name;

    std::vector<std::pair<GLfloat, glm::vec3>> keyTranslations;
    std::vector<std::pair<GLfloat, glm::quat>> keyRotations;
    std::vector<std::pair<GLfloat, glm::vec3>> keyScales;

    AnimationNode(const std::string &name)
      : name(name)
    { }

    AnimationNode() = default;
  };

  struct SceneNode
  {
    std::string name;
    std::vector<std::string> childNames;;

    glm::mat4 localTransform;

    SceneNode(const std::string &name, const glm::mat4 &localTransform)
      : name(name)
      , localTransform(localTransform)
    { }

    SceneNode() = default;
  };

  class Animation
  {
  public:
    Animation(const aiAnimation* animation, Model* parentModel);
    Animation(Model* parentModel);
    ~Animation();

    void loadAnimation(const aiAnimation* animation);

    GLfloat getDuration() const { return this->duration; }
    GLfloat getTPS() const { return this->ticksPerSecond; }
    std::string getName() const { return this->name; }
    std::unordered_map<std::string, AnimationNode>& getAniNodes() { return this->animationNodes; }
  private:
    Model* parentModel;

    std::unordered_map<std::string, AnimationNode> animationNodes;

    std::string name;
    GLfloat duration;
    GLfloat ticksPerSecond;
  };
}
