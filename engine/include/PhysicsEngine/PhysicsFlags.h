#pragma once

// Project includes
#include "Core/ApplicationBase.h"

namespace Strontium::PhysicsEngine
{
  enum class RigidBodyTypes : ushort
  {
	Static = 0u,
	Kinematic = 1u,
	Dynamic = 2u
  };

  enum class ColliderTypes : ushort
  {
	Sphere = 0u,
	Box = 1u,
	Cylinder = 2u,
	Capsule = 3u
  };

  struct UpdatedTransformData
  {
	glm::vec3 translation;
	glm::quat rotation;

	UpdatedTransformData(const glm::vec3 &translation, 
						 const glm::quat &rotation)
	  : translation(translation)
	  , rotation(rotation)
	{ }
  };
}