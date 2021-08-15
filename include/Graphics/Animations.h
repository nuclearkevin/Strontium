// Maximum bones which can influence a vertex.
#define MAX_BONES_PER_VERTEX 4
// Maximum bones in a single model file. Using SSBOs to pass them to the skinning shaders.
#define MAX_BONES_PER_MODEL 512

#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/AssetManager.h"

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

    void computeBoneTransforms(GLfloat aniTime, std::vector<glm::mat4> &outBones);

    GLfloat getDuration() const { return this->duration; }
    GLfloat getTPS() const { return this->ticksPerSecond; }
    std::string getName() const { return this->name; }
    std::unordered_map<std::string, AnimationNode>& getAniNodes() { return this->animationNodes; }
  private:
    glm::mat4 interpolateTranslation(GLfloat aniTime, const AnimationNode &node);
    glm::mat4 interpolateRotation(GLfloat aniTime, const AnimationNode &node);
    glm::mat4 interpolateScale(GLfloat aniTime, const AnimationNode &node);

    void readNodeHierarchy(GLfloat aniTime, const SceneNode &node,
                           const glm::mat4 parentTransform,
                           std::vector<glm::mat4> &outBones);
    Model* parentModel;

    std::unordered_map<std::string, AnimationNode> animationNodes;

    std::string name;
    GLfloat duration;
    GLfloat ticksPerSecond;
  };

  class Animator
  {
  public:
    Animator();
    ~Animator() = default;

    void setAnimation(Animation* animation, const AssetHandle &modelHandle);

    void onUpdate(GLfloat dt);

    void startAnimation() { this->animating = true; }
    void stopAnimation() { this->animating = false; }
    void resetAnimation() { this->currentAniTime = 0.0f; }

    std::vector<glm::mat4>& getFinalBoneTransforms() { return this->finalBoneTransforms; }
    Animation* getStoredAnimation() { return this->storedAnimation; }
    GLfloat& getAnimationTime() { return this->currentAniTime; }
    bool isAnimating() { return this->animating; }
    bool animationRenderable() { return this->storedAnimation != nullptr && this->animating; }
  private:
    GLfloat currentAniTime;
    AssetHandle storedModel;
    Animation* storedAnimation;
    std::vector<glm::mat4> finalBoneTransforms;

    bool animating;
  };
}
