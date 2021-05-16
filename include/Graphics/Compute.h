#pragma once

#include "SciRenderPCH.h"

namespace SciRenderer
{
  //----------------------------------------------------------------------------
  // The compute shader class.
  //----------------------------------------------------------------------------
  class ComputeShader
  {
  public:
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
