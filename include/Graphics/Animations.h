#pragma once

#define NUM_BONES_PER_VEREX 4
#define MAX_BONES 256

// Macro include file.
#include "StrontiumPCH.h"

// Assimp includes.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Strontium
{
  class Model;

  struct BoneData
  {
    std::string name;
    glm::mat4 boneOffsetMatrix;

    BoneData(const glm::mat4 &boneOffsetMatrix, const std::string &name)
      : name(name)
      , boneOffsetMatrix(boneOffsetMatrix)
    { }

    BoneData()
      : name("")
      , boneOffsetMatrix(1.0f)
    { }

    BoneData(BoneData&&) = default;
  };

  template<typename T>
  struct KeyframeData
  {
    T data;
    GLfloat timestamp;
    KeyframeData(const T &data, GLfloat timestamp)
      : data(data)
      , timestamp(timestamp)
    { }
  };

  // TODO: Store parent? Probably not needed since you only need to traverse
  // down the animation tree.
  struct AnimationNode
  {
    glm::mat4 transformation;
    std::string name;
    std::vector<AnimationNode> children;

    AnimationNode(const glm::mat4 &transformation, const std::string &name)
      : transformation(transformation)
      , name(name)
    { }

    AnimationNode()
      : transformation(1.0f)
      , name("")
    { }
  };

  class Bone
  {
  public:
    Bone(const std::string &name, const glm::mat4 &boneOffsetMatrix, const GLint id,
         const std::string &parentMesh);
    ~Bone() = default;
    Bone(Bone&&) = default;

    void updateTransform(GLfloat currentTime);

    std::string getName() const { return this->name; }
    GLint getID() const { return this->id; }
    std::string getParentMeshName() const { return this->parentMesh; }
    glm::mat4 getLocalTransform() { return this->localTransform; }
    glm::mat4 getOffsetMatrix() { return this->boneOffsetMatrix; }

    GLuint getNumKeyTranslations() { return this->keyTranslations.size(); }
    GLuint getNumKeyRotations() { return this->keyRotations.size(); }
    GLuint getNumKeyScales() { return this->keyScales.size(); }

    std::vector<KeyframeData<glm::vec3>>& getKeyTranslations() { return this->keyTranslations; }
    std::vector<KeyframeData<glm::quat>>& getKeyRotations() { return this->keyRotations; }
    std::vector<KeyframeData<glm::vec3>>& getKeyScales() { return this->keyScales; }
  private:
    glm::mat4 interpolateTranslation(GLfloat currentTime);
    glm::mat4 interpolateRotation(GLfloat currentTime);
    glm::mat4 interpolateScale(GLfloat currentTime);

    GLfloat computeInterpFactor(GLfloat previousTimestamp, GLfloat nextTimestamp,
                                GLfloat currentTime);

    glm::mat4 localTransform;
    glm::mat4 boneOffsetMatrix;
    GLint id;
    std::string parentMesh;
    std::string name;
    std::vector<KeyframeData<glm::vec3>> keyTranslations;
    std::vector<KeyframeData<glm::quat>> keyRotations;
    std::vector<KeyframeData<glm::vec3>> keyScales;
  };

  class Animation
  {
  public:
    Animation(const aiScene* scene, GLuint animationIndex, Model* parentModel);
    ~Animation() = default;
    Animation(Animation&&) = default;

    void onUpdate(float dt);

    std::string getName() const { return this->name; }
    std::vector<glm::mat4>& getFinalBoneMatrices(const std::string &submeshName)
      { return this->finalBoneMatrices.at(submeshName); }
  private:
    void loadKeyframeBones(const aiAnimation* animation);
    void readTransformHierarchy(AnimationNode &destNode, const aiNode* srcNode);

    void computeBoneTransforms(const AnimationNode &node, const glm::mat4 &parentTransform);

    // Animation specific data.
    Model* parentModel;
    AnimationNode rootNode;
    std::vector<Bone> animationBones;
    GLfloat duration;
    GLfloat tps;
    std::string name;

    // Animation playing data.
    GLfloat currentTime;

    // Mesh specific data.
    std::unordered_map<std::string, std::vector<glm::mat4>> finalBoneMatrices;
  };
}
