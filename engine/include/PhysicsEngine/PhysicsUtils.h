#pragma once

#include "StrontiumPCH.h"

// Jolt includes.
#include "Jolt/Jolt.h"

namespace Strontium::PhysicsUtils
{
  static 
  glm::vec3 convertJoltToGLM(JPH::Vec3 inVec)
  {
	return glm::vec3(inVec.GetX(), inVec.GetY(), inVec.GetZ());
  }

  static 
  glm::quat convertJoltToGLM(JPH::Quat inQuat)
  {
	return glm::quat(inQuat.GetW(), inQuat.GetZ(), inQuat.GetY(), inQuat.GetX());
  }

  static
  JPH::Vec3 convertGLMToJolt(const glm::vec3 &inVec)
  {
	return JPH::Vec3(inVec.x , inVec.y, inVec.z);
  }

  static 
  JPH::Quat convertGLMToJolt(const glm::quat &inQuat)
  {
	return JPH::Quat(inQuat.x, inQuat.y, inQuat.z, inQuat.w);
  }
}