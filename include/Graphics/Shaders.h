// This file contains some shader code developed by Dr. Mark Green at OTU. It
// has been heavily modified to support OpenGL abstraction.

// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

namespace SciRenderer
{
  enum class AttribType { Vec4 = 4, Vec3 = 3, Vec2 = 2};
  enum class UniformType
  {
    Float = GL_FLOAT, Vec2 = GL_FLOAT_VEC2, Vec3 = GL_FLOAT_VEC3,
    Vec4 = GL_FLOAT_VEC4, Mat3 = GL_FLOAT_MAT3, Mat4 = GL_FLOAT_MAT4,
    Sampler1D = GL_SAMPLER_1D, Sampler2D = GL_SAMPLER_2D,
    Sampler3D = GL_SAMPLER_3D, SamplerCube = GL_SAMPLER_CUBE,
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

    // Program linker function.
    void buildProgram(GLuint first, ...);

    // Bind/unbind the shader.
    void bind();
    void unbind();

    // Setters for shader uniforms.
    void addUniformMatrix(const char* uniformName, const glm::mat4 &matrix,
                          GLboolean transpose);
    void addUniformMatrix(const char* uniformName, const glm::mat3 &matrix,
                          GLboolean transpose);
    void addUniformMatrix(const char* uniformName, const glm::mat2 &matrix,
                          GLboolean transpose);
    void addUniformVector(const char* uniformName, const glm::vec4 &vector);
    void addUniformVector(const char* uniformName, const glm::vec3 &vector);
    void addUniformVector(const char* uniformName, const glm::vec2 &vector);
    void addUniformFloat(const char* uniformName, GLfloat value);
    void addUniformUInt(const char* uniformName, GLuint value);

    void addUniformSampler2D(const char* uniformName, GLuint texID);

    // Setters for vertex attributes.
    void addAtribute(const char* attribName, AttribType type,
                     GLboolean normalized, unsigned size, unsigned stride);

    // Shader dumb method, returns a string with all of the shader properties.
    std::string dumpProgram();

    // Getters.
    GLuint getShaderID() { return this->progID; }
    std::string& getInfoString() { return this->shaderInfoString; }
    std::vector<std::pair<std::string, UniformType>>& getUniforms() { return this->uniforms; }
    std::vector<std::string>& getUniformNames() { return this->uniformNames; }

    // Convert GLenums to strings.
    static std::string enumToString(GLenum sEnum);
    static UniformType enumToUniform(GLenum sEnum);
  protected:
    GLuint progID;
    GLuint vertID;
    GLuint fragID;

    std::string shaderInfoString;
    std::vector<std::pair<std::string, UniformType>> uniforms;
    std::vector<std::string> uniformNames;
  private:
      char *readShaderFile(const char* filename);
  };
}
