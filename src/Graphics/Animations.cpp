#include "Graphics/Animations.h"

// Project includes.
#include "Utils/AssimpUtilities.h"

namespace Strontium
{
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
        this->animationNodes[node->mNodeName.C_Str()] = AnimationNode(node->mNodeName.C_Str());

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
}
