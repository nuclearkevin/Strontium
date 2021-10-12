#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

namespace Strontium
{
  enum class ShaderStage
  {
    Vertex = 0x8B31, // GL_VERTEX_SHADER
    TessControl = 0x8E88, // GL_TESS_CONTROL_SHADER
    TessEval = 0x8E87, // GL_TESS_EVALUATION_SHADER
    Geometry = 0x8DD9, // GL_GEOMETRY_SHADER
    Fragment = 0x8B30, // GL_FRAGMENT_SHADER
    Compute = 0x91B9, // GL_COMPUTE_SHADER
    Unknown
  };

  enum class AttribType { Vec4, Vec3, Vec2, IVec4, IVec3, IVec2 };
  enum class UniformType
  {
    Float = 0x1406, // GL_FLOAT
    Vec2 = 0x8B50, // GL_FLOAT_VEC2
    Vec3 = 0x8B51, // GL_FLOAT_VEC3
    Vec4 = 0x8B52, // GL_FLOAT_VEC4
    Mat3 = 0x8B5B, // GL_FLOAT_MAT3
    Mat4 = 0x8B5C, // GL_FLOAT_MAT4
    Sampler1D = 0x8B5D, // GL_SAMPLER_1D
    Sampler2D = 0x8B5E, // GL_SAMPLER_2D
    Sampler3D = 0x8B5F, // GL_SAMPLER_3D
    SamplerCube = 0x8B60, // GL_SAMPLER_CUBE
    Unknown
  };

  enum class MemoryBarrierType
  {
    ShaderImageAccess = 0x00000020 // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
  };

  // Shader abstraction which supports multiple shader stages.
  class Shader
  {
  public:
    // Convert enums to strings and other enums.
    static std::string shaderStageToString(const ShaderStage &stage);

    // Compute shader memory barriers.
    static void memoryBarrier(const MemoryBarrierType &type);

    Shader();
    Shader(const std::string &filepath);
    ~Shader();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Reload the shader from its source on disk.
    void rebuild();

    void bind();
    void unbind();

    void launchCompute(uint globalX, uint globalY, uint globalZ);

    // Setters for shader uniforms.
    void addUniformMatrix(const char* uniformName, const glm::mat4 &matrix,
                          bool transpose);
    void addUniformMatrix(const char* uniformName, const glm::mat3 &matrix,
                          bool transpose);
    void addUniformMatrix(const char* uniformName, const glm::mat2 &matrix,
                          bool transpose);
    void addUniformVector(const char* uniformName, const glm::vec4 &vector);
    void addUniformVector(const char* uniformName, const glm::vec3 &vector);
    void addUniformVector(const char* uniformName, const glm::vec2 &vector);
    void addUniformFloat(const char* uniformName, float value);
    void addUniformInt(const char* uniformName, int value);
    void addUniformUInt(const char* uniformName, uint value);

    void addUniformSampler(const char* uniformName, uint texID);

  private:
    void loadFile(const std::string &filepath);
    uint compileStage(const ShaderStage &stage, const std::string &stageSource);
    void linkProgram(const std::vector<uint> &binaries);

    uint progID;

    std::unordered_map<ShaderStage, std::string> shaderSources;
    std::string shaderPath;
  };
}
