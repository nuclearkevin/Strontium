#include "Graphics/Animations.h"

// Project includes.
#include "Graphics/Model.h"
#include "Utils/AssimpUtilities.h"

namespace Strontium
{
  //----------------------------------------------------------------------------
  // Bone class starts here.
  //----------------------------------------------------------------------------
  Bone::Bone(const std::string &name, const glm::mat4 &boneOffsetMatrix, const GLint id,
             const std::string &parentMesh)
    : name(name)
    , localTransform(1.0f)
    , boneOffsetMatrix(boneOffsetMatrix)
    , id(id)
    , parentMesh(parentMesh)
  { }

  void
  Bone::updateTransform(GLfloat currentTime)
  {
    this->localTransform = this->interpolateTranslation(currentTime)
                           * this->interpolateRotation(currentTime)
                           * this->interpolateScale(currentTime);
  }

  glm::mat4
  Bone::interpolateTranslation(GLfloat currentTime)
  {
    if (this->keyTranslations.size() == 1)
      return glm::translate(glm::mat4(1.0f), this->keyTranslations[0].data);

    unsigned int previousIndex = -1;
    for (unsigned int i = 0; i < this->keyTranslations.size() - 1; i++)
    {
      if (currentTime < this->keyTranslations[i + 1].timestamp)
        previousIndex = i;
    }
    assert(("Invalid animation!", previousIndex < 0));

    unsigned nextIndex = previousIndex + 1;
    GLfloat interpFactor = this->computeInterpFactor(this->keyTranslations[previousIndex].timestamp,
                                                     this->keyTranslations[nextIndex].timestamp,
                                                     currentTime);
    glm::vec3 translation = glm::mix(this->keyTranslations[previousIndex].data,
                                     this->keyTranslations[nextIndex].data,
                                     interpFactor);
    return glm::translate(glm::mat4(1.0f), translation);
  }

  glm::mat4
  Bone::interpolateRotation(GLfloat currentTime)
  {
    if (this->keyRotations.size() == 1)
      return glm::toMat4(glm::normalize(this->keyRotations[0].data));

    unsigned int previousIndex = -1;
    for (unsigned int i = 0; i < this->keyRotations.size() - 1; i++)
    {
      if (currentTime < this->keyRotations[i + 1].timestamp)
        previousIndex = i;
    }
    assert(("Invalid animation!", previousIndex < 0));

    unsigned nextIndex = previousIndex + 1;
    GLfloat interpFactor = this->computeInterpFactor(this->keyRotations[previousIndex].timestamp,
                                                     this->keyRotations[nextIndex].timestamp,
                                                     currentTime);
    glm::quat finalRotation = glm::slerp(this->keyRotations[previousIndex].data,
                                         this->keyRotations[nextIndex].data,
                                         interpFactor);
    return glm::toMat4(glm::normalize(finalRotation));
  }

  glm::mat4
  Bone::interpolateScale(GLfloat currentTime)
  {
    if (this->keyScales.size() == 1)
      return glm::scale(glm::mat4(1.0f), this->keyScales[0].data);

    unsigned int previousIndex = -1;
    for (unsigned int i = 0; i < this->keyScales.size() - 1; i++)
    {
      if (currentTime < this->keyScales[i + 1].timestamp)
        previousIndex = i;
    }
    assert(("Invalid animation!", previousIndex < 0));

    unsigned nextIndex = previousIndex + 1;
    GLfloat interpFactor = this->computeInterpFactor(this->keyScales[previousIndex].timestamp,
                                                     this->keyScales[nextIndex].timestamp,
                                                     currentTime);
    glm::vec3 finalScale = glm::mix(this->keyScales[previousIndex].data,
                                    this->keyScales[nextIndex].data,
                                    interpFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
  }

  GLfloat
  Bone::computeInterpFactor(GLfloat previousTimestamp, GLfloat nextTimestamp,
                            GLfloat currentTime)
  {
    return (currentTime - previousTimestamp) / (nextTimestamp - previousTimestamp);
  }

  //----------------------------------------------------------------------------
  // Animation class starts here.
  //----------------------------------------------------------------------------
  Animation::Animation(const aiScene* scene, GLuint animationIndex, Model* parentModel)
    : parentModel(parentModel)
    , currentTime(0.0f)
  {
    // Get information about the animation.
    auto animation = scene->mAnimations[animationIndex];
    this->duration = animation->mDuration;
    this->tps = animation->mTicksPerSecond;

    // Load in the transform hierarchy.
    this->readTransformHierarchy(this->rootNode, scene->mRootNode);
    this->loadKeyframeBones(animation);

    // Initialize the final bone matrix arrays (one for each submesh).
    auto& submeshes = parentModel->getSubmeshes();
    for (auto& submesh : submeshes)
    {
      this->finalBoneMatrices.emplace(submesh.getName(), std::vector<glm::mat4>());
      auto& submeshBones = this->finalBoneMatrices[submesh.getName()];

      submeshBones.reserve(MAX_BONES);
      for (unsigned int i = 0; i < MAX_BONES; i++)
        submeshBones.emplace_back(1.0f);
    }

    // Debug output. TODO: Remove
    std::cout << "Animation name: " << this->name << std::endl;
    std::cout << "Bones: " << std::endl;
    for (auto& bone : this->animationBones)
    {
      std::cout << bone.getName() << " ";
      std::cout << bone.getID() << " ";
      std::cout << bone.getParentMeshName() << std::endl;
    }
  }

  void
  Animation::onUpdate(float dt)
  {
    this->currentTime += this->tps * dt;
    this->currentTime = fmod(this->currentTime, this->duration);
    this->computeBoneTransforms(this->rootNode, glm::mat4(1.0f));
  }

  void
  Animation::loadKeyframeBones(const aiAnimation* animation)
  {
    this->animationBones.reserve(animation->mNumChannels);
    for (unsigned int i = 0; i < animation->mNumChannels; i++)
    {
      auto tempMatrix = glm::mat4(1.0f);
      GLint id = -1;
      std::string parentMesh = "";

      auto boneNode = animation->mChannels[i];
      std::string nodeName = boneNode->mNodeName.C_Str();

      bool earlyOut = false;
      for (auto& submesh : this->parentModel->getSubmeshes())
      {
        auto& submeshBones = submesh.getBones();
        for (unsigned int j = 0; j < submeshBones.size(); j++)
        {
          if (submeshBones[j].name == nodeName)
          {
            tempMatrix = submeshBones[j].boneOffsetMatrix;
            id = j;
            parentMesh = submesh.getName();
            earlyOut = true;
            break;
          }
        }
        if (earlyOut)
          break;
      }

      this->animationBones.emplace_back(nodeName, tempMatrix, id, parentMesh);
      auto& translations = this->animationBones.back().getKeyTranslations();
      auto& rotations = this->animationBones.back().getKeyRotations();
      auto& scales = this->animationBones.back().getKeyScales();

      // Load the translations.
      translations.reserve(boneNode->mNumPositionKeys);
      for (unsigned int j = 0; j < boneNode->mNumPositionKeys; j++)
        translations.emplace_back(Utilities::vec3Cast(boneNode->mPositionKeys[j].mValue),
                                  boneNode->mPositionKeys[j].mTime);

      // Load the rotations.
      rotations.reserve(boneNode->mNumRotationKeys);
      for (unsigned int j = 0; j < boneNode->mNumRotationKeys; j++)
        rotations.emplace_back(Utilities::quatCast(boneNode->mRotationKeys[j].mValue),
                               boneNode->mRotationKeys[j].mTime);

      // Load the scales.
      scales.reserve(boneNode->mNumScalingKeys);
      for (unsigned int j = 0; j < boneNode->mNumScalingKeys; j++)
        scales.emplace_back(Utilities::vec3Cast(boneNode->mScalingKeys[j].mValue),
                            boneNode->mScalingKeys[j].mTime);
    }
  }

  void
  Animation::readTransformHierarchy(AnimationNode &destNode, const aiNode* srcNode)
  {
    destNode.name = srcNode->mName.C_Str();
    destNode.transformation = Utilities::mat4Cast(srcNode->mTransformation);
    destNode.children.reserve(srcNode->mNumChildren);

    for (unsigned int i = 0; i < srcNode->mNumChildren; i++)
    {
      destNode.children.emplace_back();
      this->readTransformHierarchy(destNode.children.back(), srcNode->mChildren[i]);
    }
  }

  void
  Animation::computeBoneTransforms(const AnimationNode &node,
                                   const glm::mat4 &parentTransform)
  {
    auto name = node.name;
    auto nodeTransform = node.transformation;
    auto offsetMatrix = glm::mat4(1.0f);

    auto boneLoc = std::find_if(this->animationBones.begin(), this->animationBones.end(), [name](const Bone &bone)
    {
      return bone.getName() == name;
    });

    GLint id = -1;
    std::string parentMeshName = "";
    if (boneLoc != this->animationBones.end())
    {
      boneLoc->updateTransform(this->currentTime);
      id = boneLoc->getID();
      parentMeshName = boneLoc->getParentMeshName();
      nodeTransform = boneLoc->getLocalTransform();
      offsetMatrix = boneLoc->getOffsetMatrix();
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    if (id > -1)
      this->finalBoneMatrices[parentMeshName][id] = globalTransform * offsetMatrix;

    for (auto& child : node.children)
      computeBoneTransforms(child, globalTransform);
  }
}
