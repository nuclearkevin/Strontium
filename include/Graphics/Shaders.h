// This file contains some shader code developed by Dr. Mark Green at OTU. It
// has been heavily modified to support OpenGL abstraction.

// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

namespace Strontium
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
    void buildShaderSource(int type, const std::string &strSource);

    // Program linker function.
    void buildProgram(GLuint first, ...);

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

    void addUniformSampler(const char* uniformName, GLuint texID);
    GLint getSamplerLocation(const char* uniformName);
    
    // Setters for vertex attributes.
    void addAtribute(const char* attribName, AttribType type,
                     GLboolean normalized, unsigned size, unsigned stride);

    // Shader dumb method, returns a string with all of the shader properties.
    std::string dumpProgram();

    // Getters.
    GLuint getShaderID() { return this->progID; }
    std::string& getInfoString() { return this->shaderInfoString; }
    std::string& getVertSource() { return this->vertSource; }
    std::string& getFragSource() { return this->fragSource; }
    std::vector<std::pair<std::string, UniformType>>& getUniforms() { return this->uniforms; }
    std::vector<std::string>& getUniformNames() { return this->uniformNames; }

    // Set the shader source for dynamic rebuilding.
    void setVertSource(const std::string &source) { this->vertSource = source; }
    void setFragSource(const std::string &source) { this->fragPath = source; }

    // Convert GLenums to strings.
    static std::string enumToString(GLenum sEnum);
    static UniformType enumToUniform(GLenum sEnum);
  protected:
    GLuint progID;
    GLuint vertID;
    GLuint fragID;

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
