#include "Graphics/Animations.h"

// Project includes.
#include "Utils/AssimpUtilities.h"
#include "Graphics/Model.h"

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
  { }

  Animation::~Animation()
  {

  }

  void
  Animation::loadAnimation(const aiAnimation* animation)
  {
    this->name = animation->mName.C_Str();
    this->duration = animation->mDuration;
    this->ticksPerSecond = animation->mTicksPerSecond == 0.0f ? 25.0f : animation->mTicksPerSecond;

    for (unsigned int i = 0; i < animation->mNumChannels; i++)
    {
      aiNodeAnim* node = animation->mChannels[i];
      if (this->animationNodes.find(node->mNodeName.C_Str()) == this->animationNodes.end())
      {
        this->animationNodes.insert(std::make_pair<std::string, AnimationNode>(node->mNodeName.C_Str(), AnimationNode(node->mNodeName.C_Str())));

        // Load translations.
        this->animationNodes[node->mNodeName.C_Str()].keyTranslations.reserve(node->mNumPositionKeys);
        for (unsigned int j = 0; j < node->mNumPositionKeys; j++)
        {
          this->animationNodes[node->mNodeName.C_Str()]
            .keyTranslations.emplace_back(node->mPositionKeys[j].mTime, Utilities::vec3ToGLM(node->mPositionKeys[j].mValue));
        }

        // Load rotations.
        this->animationNodes[node->mNodeName.C_Str()].keyRotations.reserve(node->mNumRotationKeys);
        for (unsigned int j = 0; j < node->mNumRotationKeys; j++)
        {
          this->animationNodes[node->mNodeName.C_Str()]
            .keyRotations.emplace_back(node->mRotationKeys[j].mTime, Utilities::quatToGLM(node->mRotationKeys[j].mValue));
        }

        // Load scales.
        this->animationNodes[node->mNodeName.C_Str()].keyScales.reserve(node->mNumScalingKeys);
        for (unsigned int j = 0; j < node->mNumScalingKeys; j++)
        {
          this->animationNodes[node->mNodeName.C_Str()]
            .keyScales.emplace_back(node->mScalingKeys[j].mTime, Utilities::vec3ToGLM(node->mScalingKeys[j].mValue));
        }
      }
    }
  }

  void
  Animation::computeBoneTransforms(GLfloat aniTime, std::vector<glm::mat4>& outBones)
  {
    outBones.clear();
    outBones.resize(this->parentModel->getBones().size(), glm::mat4(1.0f));
    this->readNodeHierarchy(aniTime, this->parentModel->getRootNode(), glm::mat4(1.0f), outBones);
  }

  void
  Animation::readNodeHierarchy(GLfloat aniTime, const SceneNode &node,
                               const glm::mat4 parentTransform,
                               std::vector<glm::mat4> &outBones)
  {
    auto nodeName = node.name;
    auto nodeTransform = node.localTransform;

    if (this->animationNodes.find(nodeName) != this->animationNodes.end())
    {
      auto aniNode = this->animationNodes[nodeName];
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
      this->readNodeHierarchy(aniTime, sceneNodes[childNodeName], globalTransform, outBones);
  }

  glm::mat4
  Animation::interpolateTranslation(GLfloat aniTime, const AnimationNode &node)
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
  Animation::interpolateRotation(GLfloat aniTime, const AnimationNode &node)
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
  Animation::interpolateScale(GLfloat aniTime, const AnimationNode &node)
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
    , paused(false)
  { }

  void
  Animator::setAnimation(Animation* animation, const AssetHandle &modelHandle)
  {
    auto modelAssets = AssetManager<Model>::getManager();

    if (modelAssets->hasAsset(modelHandle))
    {
      this->storedModel = modelHandle;
      this->storedAnimation = animation;
      this->currentAniTime = 0.0f;
    }
  }

  void
  Animator::onUpdate(GLfloat dt)
  {
    if (this->storedAnimation && this->animating && !this->paused)
    {
      this->currentAniTime += dt * this->storedAnimation->getTPS();
      this->currentAniTime = fmod(this->currentAniTime, this->storedAnimation->getDuration());

      this->storedAnimation->computeBoneTransforms(this->currentAniTime, this->finalBoneTransforms);
    }
    else if (this->storedAnimation && this->animating && this->paused)
      this->storedAnimation->computeBoneTransforms(this->currentAniTime, this->finalBoneTransforms);
  }
}
