#include "Graphics/Animations.h"

// Project includes.
#include "Core/Application.h"

#include "Assets/AssetManager.h"
#include "Assets/ModelAsset.h"
#include "Utils/AssimpUtilities.h"

#include "Graphics/Model.h"

// Assimp includes.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Strontium
{
  //----------------------------------------------------------------------------
  // Animation class.
  //----------------------------------------------------------------------------
  Animation::Animation(const aiAnimation* animation, Model* parentModel)
    : parentModel(parentModel)
  {
    this->loadAnimation(animation);
  }

  Animation::Animation(Model* parentModel)
    : parentModel(parentModel)
    , duration(0.0f)
    , ticksPerSecond(0.0f)
  { }

  Animation::~Animation()
  { }

  void
  Animation::loadAnimation(const aiAnimation* animation)
  {
    this->name = animation->mName.C_Str();
    this->duration = animation->mDuration;
    this->ticksPerSecond = animation->mTicksPerSecond == 0.0f ? 25.0f : animation->mTicksPerSecond;

    for (uint i = 0; i < animation->mNumChannels; i++)
    {
      aiNodeAnim* node = animation->mChannels[i];
      if (this->animationNodes.find(node->mNodeName.C_Str()) == this->animationNodes.end())
      {
        this->animationNodes.emplace(std::string(node->mNodeName.C_Str()), AnimationNode(node->mNodeName.C_Str()));

        // Load translations.
        this->animationNodes[node->mNodeName.C_Str()].keyTranslations.reserve(node->mNumPositionKeys);
        for (uint j = 0; j < node->mNumPositionKeys; j++)
        {
          this->animationNodes[node->mNodeName.C_Str()]
            .keyTranslations.emplace_back(node->mPositionKeys[j].mTime, Utilities::vec3ToGLM(node->mPositionKeys[j].mValue));
        }

        // Load rotations.
        this->animationNodes[node->mNodeName.C_Str()].keyRotations.reserve(node->mNumRotationKeys);
        for (uint j = 0; j < node->mNumRotationKeys; j++)
        {
          this->animationNodes[node->mNodeName.C_Str()]
            .keyRotations.emplace_back(node->mRotationKeys[j].mTime, Utilities::quatToGLM(node->mRotationKeys[j].mValue));
        }

        // Load scales.
        this->animationNodes[node->mNodeName.C_Str()].keyScales.reserve(node->mNumScalingKeys);
        for (uint j = 0; j < node->mNumScalingKeys; j++)
        {
          this->animationNodes[node->mNodeName.C_Str()]
            .keyScales.emplace_back(node->mScalingKeys[j].mTime, Utilities::vec3ToGLM(node->mScalingKeys[j].mValue));
        }
      }
    }
  }

  uint 
  Animation::getNumBones() const
  { 
    return this->parentModel->getBones().size(); 
  }

  void
  Animation::computeBoneTransforms(float aniTime, std::vector<glm::mat4>& outBonesSkinned, 
                                   robin_hood::unordered_flat_map<std::string, glm::mat4> &outBonesUnskinned)
  {
    if (this->parentModel->hasSkins())
    {
      // Compute the transformations for a skinned model.
      outBonesSkinned.clear();
      outBonesSkinned.resize(this->parentModel->getBones().size(), glm::mat4(1.0f));

      this->readSkinnedNodeHierarchy(aniTime, this->parentModel->getRootNode(), 
                                     parentModel->getGlobalTransform(), outBonesSkinned);
    }
    else
    {
      // Compute the transformations for an unskinned model.
      outBonesUnskinned.clear();
      this->readUnSkinnedNodeHierarchy(aniTime, this->parentModel->getRootNode(),
                                       parentModel->getGlobalTransform(), outBonesUnskinned);
    }
  }

  void 
  Animation::readUnSkinnedNodeHierarchy(float aniTime, const SceneNode& node,
                                        const glm::mat4 parentTransform,
                                        robin_hood::unordered_flat_map<std::string, glm::mat4>& outBones)
  {
    auto nodeName = node.name;
    auto nodeTransform = node.localTransform;

    if (this->animationNodes.find(nodeName) != this->animationNodes.end())
    {
      auto& aniNode = this->animationNodes.at(nodeName);
      glm::mat4 translation = this->interpolateTranslation(aniTime, aniNode);
      glm::mat4 rotation = this->interpolateRotation(aniTime, aniNode);
      glm::mat4 scale = this->interpolateScale(aniTime, aniNode);

      nodeTransform = translation * rotation * scale;
    }

    auto globalTransform = parentTransform * nodeTransform;

    outBones.emplace(nodeName, this->parentModel->getGlobalInverseTransform() * globalTransform);

    auto& sceneNodes = this->parentModel->getSceneNodes();
    for (auto& childNodeName : node.childNames)
      this->readUnSkinnedNodeHierarchy(aniTime, sceneNodes[childNodeName], globalTransform, outBones);
  }

  void
  Animation::readSkinnedNodeHierarchy(float aniTime, const SceneNode &node,
                                      const glm::mat4 parentTransform,
                                      std::vector<glm::mat4> &outBones)
  {
    auto nodeName = node.name;
    auto nodeTransform = node.localTransform;

    if (this->animationNodes.find(nodeName) != this->animationNodes.end())
    {
      auto& aniNode = this->animationNodes.at(nodeName);
      glm::mat4 translation = this->interpolateTranslation(aniTime, aniNode);
      glm::mat4 rotation = this->interpolateRotation(aniTime, aniNode);
      glm::mat4 scale = this->interpolateScale(aniTime, aniNode);

      nodeTransform = translation * rotation * scale;
    }

    auto globalTransform = parentTransform * nodeTransform;

    auto& boneMap = this->parentModel->getBoneMap();
    if (boneMap.find(nodeName) != boneMap.end())
    {
      auto& modelBones = this->parentModel->getBones();
      unsigned int index = boneMap[nodeName];
      auto boneOffset = this->parentModel->getBones()[index].offsetMatrix;
      outBones[index] = this->parentModel->getGlobalInverseTransform() * globalTransform * boneOffset;
    }

    auto& sceneNodes = this->parentModel->getSceneNodes();
    for (auto& childNodeName : node.childNames)
      this->readSkinnedNodeHierarchy(aniTime, sceneNodes[childNodeName], globalTransform, outBones);
  }

  glm::mat4
  Animation::interpolateTranslation(float aniTime, const AnimationNode &node)
  {
    if (node.keyTranslations.size() == 1)
      return glm::translate(glm::mat4(1.0f), node.keyTranslations[0].second);

    unsigned int currentIndex = 0;
    for (unsigned int i = 0; i < node.keyTranslations.size() - 1; i++)
    {
      bool animating;
      if (aniTime < node.keyTranslations[i + 1].first)
      {
        currentIndex = i;
        break;
      }
    }
    unsigned int nextIndex = currentIndex + 1;

    float dt = node.keyTranslations[nextIndex].first - node.keyTranslations[currentIndex].first;
    float normalized = (aniTime - node.keyTranslations[currentIndex].first) / dt;
    auto translation = glm::mix(node.keyTranslations[currentIndex].second,
                                node.keyTranslations[nextIndex].second,
                                normalized);
    return glm::translate(glm::mat4(1.0f), translation);
  }

  glm::mat4
  Animation::interpolateRotation(float aniTime, const AnimationNode &node)
  {
    if (node.keyRotations.size() == 1)
      return glm::toMat4(node.keyRotations[0].second);

    unsigned int currentIndex = 0;
    for (unsigned int i = 0; i < node.keyRotations.size() - 1; i++)
    {
      if (aniTime < node.keyRotations[i + 1].first)
      {
        currentIndex = i;
        break;
      }
    }
    unsigned int nextIndex = currentIndex + 1;

    float dt = node.keyRotations[nextIndex].first - node.keyRotations[currentIndex].first;
    float normalized = (aniTime - node.keyRotations[currentIndex].first) / dt;
    auto rotation = glm::normalize(glm::slerp(node.keyRotations[currentIndex].second,
                                              node.keyRotations[nextIndex].second,
                                              normalized));
    return glm::toMat4(rotation);
  }

  glm::mat4
  Animation::interpolateScale(float aniTime, const AnimationNode &node)
  {
    if (node.keyScales.size() == 1)
      return glm::scale(glm::mat4(1.0f), node.keyScales[0].second);

    unsigned int currentIndex = 0;
    for (unsigned int i = 0; i < node.keyScales.size() - 1; i++)
    {
      if (aniTime < node.keyScales[i + 1].first)
      {
        currentIndex = i;
        break;
      }
    }
    unsigned int nextIndex = currentIndex + 1;

    float dt = node.keyScales[nextIndex].first - node.keyScales[currentIndex].first;
    float normalized = (aniTime - node.keyScales[currentIndex].first) / dt;
    auto scale = glm::mix(node.keyScales[currentIndex].second,
                          node.keyScales[nextIndex].second,
                          normalized);
    return glm::scale(glm::mat4(1.0f), scale);
  }

  //----------------------------------------------------------------------------
  // Animator class. This is stored in the renderable component.
  //----------------------------------------------------------------------------
  Animator::Animator()
    : storedModel("")
    , storedAnimation(nullptr)
    , currentAniTime(0.0f)
    , animating(false)
    , paused(true)
    , scrubbing(false)
  { }

  void
  Animator::setAnimation(Animation* animation, const Asset::Handle &modelHandle)
  {
    auto& assetCache = Application::getInstance()->getAssetCache();

    if (assetCache.has<ModelAsset>(modelHandle))
    {
      this->storedModel = modelHandle;
      this->storedAnimation = animation;
      this->currentAniTime = 0.0f;
    }
  }

  void
  Animator::onUpdate(float dt)
  {
    if (this->storedAnimation && this->animating && !this->paused)
    {
      this->currentAniTime += dt * this->storedAnimation->getTPS();
      this->currentAniTime = fmod(this->currentAniTime, this->storedAnimation->getDuration());

      this->storedAnimation->computeBoneTransforms(this->currentAniTime, this->finalBoneTransforms, 
                                                   this->unSkinnedFinalTransforms);
    }

    if (this->scrubbing)
    {
      this->scrubbing = false;
      this->storedAnimation->computeBoneTransforms(this->currentAniTime, this->finalBoneTransforms, 
                                                   this->unSkinnedFinalTransforms);
    }
  }
}
