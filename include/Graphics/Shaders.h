// This file contains some shader code developed by Dr. Mark Green at OTU. It
// has been heavily modified to support OpenGL abstraction.

// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

namespace Strontium
{
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

  class Shader
  {
  public:
    // Constructor and destructor.
    Shader();
    Shader(const std::string &vertPath, const std::string &fragPath);
    ~Shader();

    // Shader parser/compiler function.
    void buildShader(int type, const char* filename);
    void buildShaderSource(int type, const std::string &strSource);

    // Program linker function.
    void buildProgram(uint first, ...);

    // Rebuild the shader. Includes source parsing, compiling and linking.
    void rebuild();

    // Rebuilds from the shader sources as strings.
    void rebuildFromString();
    void rebuildFromString(const std::string &vertSource, const std::string &fragSource);

    // Saves the vertex and fragment shader source code to the files they were
    // loaded from.
    void saveSourceToFiles();
    // Saves the vertex and fragment shader source code to a new text file.
    void saveSourceToFiles(const std::string &vertPath, const std::string &fragPath);

    // Bind/unbind the shader.
    void bind();
    void unbind();

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
    int getSamplerLocation(const char* uniformName);

    // Setters for vertex attributes.
    void addAtribute(const char* attribName, AttribType type,
                     bool transpose, unsigned size, unsigned stride);

    // Shader dumb method, returns a string with all of the shader properties.
    std::string dumpProgram();

    // Getters.
    uint getShaderID() { return this->progID; }
    std::string& getInfoString() { return this->shaderInfoString; }
    std::string& getVertSource() { return this->vertSource; }
    std::string& getFragSource() { return this->fragSource; }
    std::vector<std::pair<std::string, UniformType>>& getUniforms() { return this->uniforms; }
    std::vector<std::string>& getUniformNames() { return this->uniformNames; }

    // Set the shader source for dynamic rebuilding.
    void setVertSource(const std::string &source) { this->vertSource = source; }
    void setFragSource(const std::string &source) { this->fragPath = source; }

    // Convert GLenums to strings.
    static std::string enumToString(uint sEnum);
    static UniformType enumToUniform(uint sEnum);
  protected:
    uint progID;
    uint vertID;
    uint fragID;

    std::string vertPath;
    std::string fragPath;

    std::string vertSource;
    std::string fragSource;

    std::string shaderInfoString;
    std::vector<std::pair<std::string, UniformType>> uniforms;
    std::vector<std::string> uniformNames;
  private:
      char *readShaderFile(const char* filename);
  };
}
