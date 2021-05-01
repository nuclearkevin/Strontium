// This file contains some shader code developed by Dr. Mark Green at OTU. It
// has been heavily modified to support OpenGL abstraction.

// Macro include file.
#include "SciRenderIncludes.h"

// STL includes.
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

// Header include.
#include "Shaders.h"

using namespace SciRenderer;

// Constructor and destructor.
Shader::Shader(const char* vertPath, const char* fragPath)
{
	this->buildShader(GL_VERTEX_SHADER, vertPath);
	this->buildShader(GL_FRAGMENT_SHADER, fragPath);
	this->buildProgram(this->vertID, this->fragID, 0);
	glUseProgram(this->progID);
}

Shader::~Shader()
{
	glDeleteProgram(this->progID);
	glDeleteShader(this->vertID);
	glDeleteShader(this->fragID);
}

void
Shader::bind()
{
	glUseProgram(this->progID);
}

void
Shader::unbind()
{
	glUseProgram(0);
}

void
Shader::buildShader(int type, const char* filename)
{
	GLuint shaderID;
	char *source;
	int result;
	char *buffer;

	shaderID = glCreateShader(type);
	source = readShaderFile(filename);
	if (source == 0)
		return;

	glShaderSource(shaderID, 1, (const  GLchar **) &source, 0);
	glCompileShader(shaderID);
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE)
	{
		printf("shader compile error: %s\n",filename);
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &result);
		buffer = new char[result];
		glGetShaderInfoLog(shaderID, result, 0, buffer);
		printf("%s\n", buffer);
		delete buffer;
	}

	switch (type)
	{
		case GL_VERTEX_SHADER:
			this->vertID = shaderID;
			break;
		case GL_FRAGMENT_SHADER:
			this->fragID = shaderID;
			break;
		default:
			printf("Shader type unknown!\n");
			break;
	}
}

void
Shader::buildProgram(GLuint first, ...)
{
	int result;
	char *buffer;
	va_list argptr;
	int shader;
	int vs = 0;
	int fs = 0;
	int type;

	this->progID = glCreateProgram();
	if (first != 0)
	{
		glAttachShader(this->progID, first);
		glGetShaderiv(first, GL_SHADER_TYPE, &type);
		if (type == GL_VERTEX_SHADER)
			vs++;
		if (type == GL_FRAGMENT_SHADER)
			fs++;
	}

	va_start(argptr,first);
	while ((shader = va_arg(argptr,int)) != 0)
	{
		glAttachShader(this->progID, shader);
		glGetShaderiv(shader, GL_SHADER_TYPE, &type);
		if (type == GL_VERTEX_SHADER)
			vs++;
		if (type == GL_FRAGMENT_SHADER)
			fs++;
	}

	if (vs == 0)
		printf("no vertex shader\n");
	if (fs == 0)
		printf("no fragment shader\n");

	glLinkProgram(this->progID);
	glGetProgramiv(this->progID, GL_LINK_STATUS, &result);

	if (result != GL_TRUE)
	{
		printf("program link error\n");
		glGetProgramiv(this->progID, GL_INFO_LOG_LENGTH, &result);
		buffer = new char[result];
		glGetProgramInfoLog(this->progID, result, 0, buffer);
		printf("%s\n",buffer);
		delete buffer;
	}
}

// Setters for uniform matrices.
void
Shader::addUniformMatrix(const char* uniformName, const glm::mat4 &matrix,
												 GLboolean transpose)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniformMatrix4fv(uniLoc, 1, transpose, glm::value_ptr(matrix));
}

void
Shader::addUniformMatrix(const char* uniformName, const glm::mat3 &matrix,
												 GLboolean transpose)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniformMatrix3fv(uniLoc, 1, transpose, glm::value_ptr(matrix));
}

void
Shader::addUniformMatrix(const char* uniformName, const glm::mat2 &matrix,
												 GLboolean transpose)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniformMatrix2fv(uniLoc, 1, transpose, glm::value_ptr(matrix));
}

// Setter for uniform vectors.
void
Shader::addUniformVector(const char* uniformName, const glm::vec4 &vector)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniform4f(uniLoc, vector[0], vector[1], vector[2], vector[3]);
}

void
Shader::addUniformVector(const char* uniformName, const glm::vec3 &vector)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniform3f(uniLoc, vector[0], vector[1], vector[2]);
}

void
Shader::addUniformVector(const char* uniformName, const glm::vec2 &vector)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniform2f(uniLoc, vector[0], vector[1]);
}

// Setters for singleton uniform data.
void
Shader::addUniformFloat(const char* uniformName, GLfloat value)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniform1f(uniLoc, value);
}

void
Shader::addUniformUInt(const char* uniformName, GLuint value)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniform1ui(uniLoc, value);
}

// Set a texture sampler.
void
Shader::addUniformSampler2D(const char* uniformName, GLuint texID)
{
	this->bind();
	GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
	glUniform1i(uniLoc, texID);
}

// Vertex attribute setters.
void
Shader::addAtribute(const char* attribName, GLVectorType type,
										GLboolean normalized, unsigned size, unsigned stride)
{
	this->bind();
	GLuint attribPos = glGetAttribLocation(this->progID, attribName);
	glVertexAttribPointer(attribPos, type, GL_FLOAT, normalized, size,
												(void*)stride);
	glEnableVertexAttribArray(attribPos);
}

// Getters, yay. . .
GLuint
Shader::getShaderID() {return this->progID;}

// Debug method to dump the shader program.
void
Shader::dumpProgram(GLuint program, char* description)
{
	char name[256];
	GLsizei length;
	GLint size;
	GLenum type;
	int uniforms;
	int attributes;
	int shaders;
	int i;

	printf("Information for shader: %s\n",description);

	if (!glIsProgram(program))
	{
		printf("not a valid shader program\n");
		return;
	}

	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shaders);
	printf("Number of shaders: %d\n",shaders);

	glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&uniforms);
	printf("uniforms: %d\n",uniforms);

	for (i=0; i<uniforms; i++)
	{
		glGetActiveUniform(program, i, 256, &length ,&size ,&type, name);
		printf("  name: %s\n",name);
	}

	glGetProgramiv(program,GL_ACTIVE_ATTRIBUTES,&attributes);
	printf("attributes: %d\n",attributes);

	for (i=0; i<attributes; i++)
	{
		glGetActiveAttrib(program, i, 256, &length, &size, &type, name);
		printf("  name: %s\n",name);
	}
}

char*
Shader::readShaderFile(const char* filename)
{
	FILE* fid;
	char *buffer;
	int len;
	int n;

	fid = fopen(filename,"r");
	if (fid == NULL)
	{
		printf("can't open shader file: %s\n", filename);
		return(0);
	}

	fseek(fid, 0, SEEK_END);
	len = ftell(fid);
	rewind(fid);

	buffer = new char[len+1];
	n = fread(buffer, sizeof(char), len, fid);
	buffer[n] = 0;

	return buffer;
}
