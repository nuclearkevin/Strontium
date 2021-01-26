// This file contains some shader code developed by Dr. Mark Green at OTU. It
// has been heavily modified to support OpenGL abstraction.

// Include guard.
#pragma once

// OpenGL includes.
#include <GL/glew.h>
#include <glm/glm.hpp>

enum GLVectorType { VEC4 = 4, VEC3 = 3, VEC2 = 1};

class Shader
{
public:
  // Constructor and destructor.
  Shader(const char* vertPath, const char* fragPath);
  ~Shader();

  // Forward declaration of the shader parser/compiler function.
  void buildShader(int type, const char* filename);

  // Forward declaration of the program linker function.
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

  // Setters for vertex attributes.
  void addAtribute(const char* attribName, GLVectorType type,
                   GLboolean normalized, unsigned size, unsigned stride);

  // Getters.
  GLuint getShaderID();

  // Forward declaration of the shader debug function.
  void dumpProgram(GLuint program, char* description);
protected:
    GLuint progID;
    GLuint vertID;
    GLuint fragID;

private:
    char *readShaderFile(const char* filename);
};
