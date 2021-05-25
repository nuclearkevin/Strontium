// This file contains some shader code developed by Dr. Mark Green at OTU. It
// has been heavily modified to support OpenGL abstraction.

// Header include.
#include "Graphics/Shaders.h"

namespace SciRenderer
{
	// Constructor and destructor.
	Shader::Shader()
	{
		this->progID = glCreateProgram();
		this->vertID = glCreateShader(GL_VERTEX_SHADER);
		this->fragID = glCreateShader(GL_FRAGMENT_SHADER);
	}

	Shader::Shader(const std::string &vertPath, const std::string &fragPath)
	{
		this->buildShader(GL_VERTEX_SHADER, vertPath.c_str());
		this->buildShader(GL_FRAGMENT_SHADER, fragPath.c_str());
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

	// Build and validate a shader program. TODO: Move away from C to C++.
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

		glShaderSource(shaderID, 1, (const  GLchar**) &source, 0);
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

	// Link the shader program together. TODO: Move away from C to C++.
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
		// Cast to long first to avoid a compiler warning on 64 bit systems.
		glVertexAttribPointer(attribPos, type, GL_FLOAT, normalized, size,
													(void*) (unsigned long) stride);
		glEnableVertexAttribArray(attribPos);
	}

	// Debug method to dump the shader program. TODO: Move away from C to C++.
	void
	Shader::dumpProgram(char* description)
	{
		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		int uniforms, attributes, shaders;

		printf("Information for shader: %s\n", description);

		if (!glIsProgram(this->progID))
		{
			printf("Not a valid shader program!\n");
			return;
		}

		glGetProgramiv(this->progID, GL_ATTACHED_SHADERS, &shaders);
		printf("Number of shaders: %d\n", shaders);

		glGetProgramiv(this->progID, GL_ACTIVE_UNIFORMS, &uniforms);
		printf("Number of uniforms: %d\n", uniforms);

		for (unsigned i = 0; i < uniforms; i++)
		{
			glGetActiveUniform(this->progID, i, 256, &length, &size, &type, name);
			printf("  Name: %s\n", name);
		}

		glGetProgramiv(this->progID, GL_ACTIVE_ATTRIBUTES, &attributes);
		printf("Attributes: %d\n", attributes);

		for (unsigned i = 0; i < attributes; i++)
		{
			glGetActiveAttrib(this->progID, i, 256, &length, &size, &type, name);
			printf("  Name: %s\n", name);
		}
	}

  // Read in the shader source code. TODO: Move away from C to C++.
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
}
