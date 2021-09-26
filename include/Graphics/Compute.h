#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

namespace Strontium
{
  enum class MemoryBarrierType
  {
    ShaderImageAccess = 0x00000020 // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
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
    void dumpProgram(uint program, char* description);

    // Shader parser/compiler function.
    void buildShader(int type, const char* filename);

    // Program linker function.
    void buildProgram();
  protected:
    uint progID;
    uint computeID;
  private:
      char* readShaderFile(const char* filename);
  };
}
