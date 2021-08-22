#pragma once

#include "StrontiumPCH.h"

namespace Strontium
{
  enum class MemoryBarrierType
  {
    ShaderImageAccess = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
  };

  //----------------------------------------------------------------------------
  // The compute shader class.
  //----------------------------------------------------------------------------
  class ComputeShader
  {
  public:
    static void memoryBarrier(const MemoryBarrierType &type);

    ComputeShader(const std::string &filepath);
    ~ComputeShader();

    // Bind/unbind the shader program.
    void bind();
    void unbind();

    // Launch the compute operation.
    void launchCompute(const glm::ivec3 &computeUnits);

    // Forward declaration of the shader debug function.
    void dumpProgram(GLuint program, char* description);

    // Shader parser/compiler function.
    void buildShader(int type, const char* filename);

    // Program linker function.
    void buildProgram();
  protected:
    GLuint progID;
    GLuint computeID;
  private:
      char* readShaderFile(const char* filename);
  };
}
