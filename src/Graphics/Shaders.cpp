// This file contains some shader code developed by Dr. Mark Green at OTU. It
// has been heavily modified to support OpenGL abstraction.
#include "Graphics/Shaders.h"

// Project includes.
#include "Core/Logs.h"

namespace Strontium
{
	// Constructor and destructor.
	Shader::Shader()
	{
		this->progID = glCreateProgram();
		this->vertID = glCreateShader(GL_VERTEX_SHADER);
		this->fragID = glCreateShader(GL_FRAGMENT_SHADER);
	}

	Shader::Shader(const std::string &vertPath, const std::string &fragPath)
		: vertPath(vertPath)
		, fragPath(fragPath)
	{
		// Build the shader from source.
		this->buildShader(GL_VERTEX_SHADER, vertPath.c_str());
		this->buildShader(GL_FRAGMENT_SHADER, fragPath.c_str());
		this->buildProgram(this->vertID, this->fragID, 0);
		glUseProgram(this->progID);

		// Reflect the shader and gather a list of uniforms.
		this->shaderInfoString = this->dumpProgram();

		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		int uniforms;

		// Generates a list of uniforms based on their name and type.
		glGetProgramiv(this->progID, GL_ACTIVE_UNIFORMS, &uniforms);
		for (unsigned i = 0; i < uniforms; i++)
		{
			glGetActiveUniform(this->progID, i, 256, &length, &size, &type, name);
			this->uniforms.push_back({ name, enumToUniform(type) });
		}
	}

	Shader::~Shader()
	{
		glDeleteProgram(this->progID);
		glDeleteShader(this->vertID);
		glDeleteShader(this->fragID);
	}

	void
	Shader::rebuild()
	{
		// Delete the old shader.
		glDeleteProgram(this->progID);
		glDeleteShader(this->vertID);
		glDeleteShader(this->fragID);

		// Build the shader from source.
		this->buildShader(GL_VERTEX_SHADER, this->vertPath.c_str());
		this->buildShader(GL_FRAGMENT_SHADER, this->fragPath.c_str());
		this->buildProgram(this->vertID, this->fragID, 0);
		glUseProgram(this->progID);

		// Reflect the shader and gather a list of uniforms.
		this->shaderInfoString = this->dumpProgram();

		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		int uniforms;

		// Generates a list of uniforms based on their name and type.
		this->uniforms.clear();
		glGetProgramiv(this->progID, GL_ACTIVE_UNIFORMS, &uniforms);
		for (unsigned i = 0; i < uniforms; i++)
		{
			glGetActiveUniform(this->progID, i, 256, &length, &size, &type, name);
			this->uniforms.push_back({ name, enumToUniform(type) });
		}
	}

	void
	Shader::rebuildFromString()
	{
		this->buildShaderSource(GL_VERTEX_SHADER, this->vertSource);
		this->buildShaderSource(GL_FRAGMENT_SHADER, this->fragSource);
		this->buildProgram(this->vertID, this->fragID, 0);
		glUseProgram(this->progID);

		// Reflect the shader and gather a list of uniforms.
		this->shaderInfoString = this->dumpProgram();

		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		int uniforms;

		// Generates a list of uniforms based on their name and type.
		this->uniforms.clear();
		glGetProgramiv(this->progID, GL_ACTIVE_UNIFORMS, &uniforms);
		for (unsigned i = 0; i < uniforms; i++)
		{
			glGetActiveUniform(this->progID, i, 256, &length, &size, &type, name);
			this->uniforms.push_back({ name, enumToUniform(type) });
		}
	}

	void
	Shader::rebuildFromString(const std::string &vertSource,
														const std::string &fragSource)
	{
		this->buildShaderSource(GL_VERTEX_SHADER, vertSource);
		this->buildShaderSource(GL_FRAGMENT_SHADER, fragSource);
		this->buildProgram(this->vertID, this->fragID, 0);
		glUseProgram(this->progID);

		// Reflect the shader and gather a list of uniforms.
		this->shaderInfoString = this->dumpProgram();

		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		int uniforms;

		// Generates a list of uniforms based on their name and type.
		this->uniforms.clear();
		glGetProgramiv(this->progID, GL_ACTIVE_UNIFORMS, &uniforms);
		for (unsigned i = 0; i < uniforms; i++)
		{
			glGetActiveUniform(this->progID, i, 256, &length, &size, &type, name);
			this->uniforms.push_back({ name, enumToUniform(type) });
		}
	}

	// Build and validate a shader program. TODO: Move away from C to C++.
	void
	Shader::buildShader(int type, const char* filename)
	{
		Logger* logs = Logger::getInstance();

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
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &result);
			buffer = new char[result];
			glGetShaderInfoLog(shaderID, result, 0, buffer);
			logs->logMessage(LogMessage(std::string("Shader compiler error at: ")
																	+ std::string(filename) + std::string("\n")
																	+ std::string(buffer), true, true));
			delete buffer;
		}

		switch (type)
		{
			case GL_VERTEX_SHADER:
				this->vertID = shaderID;
				this->vertSource = std::string(source);
				break;
			case GL_FRAGMENT_SHADER:
				this->fragID = shaderID;
				this->fragSource = std::string(source);
				break;
			default:
				logs->logMessage(LogMessage("Shader type unknown!", true, true));
				break;
		}
	}

	// Builds a shader from the provided source.
	void
	Shader::buildShaderSource(int type, const std::string &strSource)
	{
		Logger* logs = Logger::getInstance();

		GLuint shaderID;
		int result;
		char* buffer;

		shaderID = glCreateShader(type);
		char* source = (char*) strSource.c_str();
		if (source == 0)
			return;

		glShaderSource(shaderID, 1, (const  GLchar**) &source, 0);
		glCompileShader(shaderID);
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
		if (result != GL_TRUE)
		{
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &result);
			buffer = new char[result];
			glGetShaderInfoLog(shaderID, result, 0, buffer);
			logs->logMessage(LogMessage(std::string("Shader compiler error: ")
																	+ std::string(buffer), true, true));
			delete buffer;
		}

		switch (type)
		{
			case GL_VERTEX_SHADER:
				this->vertID = shaderID;
				this->vertSource = std::string(source);
				break;
			case GL_FRAGMENT_SHADER:
				this->fragID = shaderID;
				this->fragSource = std::string(source);
				break;
			default:
				logs->logMessage(LogMessage("Shader type unknown!", true, true));
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

	// Saves the vertex and fragment shader source code to the files they were
	// loaded from.
	void
	Shader::saveSourceToFiles()
	{
		Logger* logs = Logger::getInstance();

		if (this->vertPath == "" || this->fragPath == "")
		{
			logs->logMessage(LogMessage("Could not save shader file, missing one or "
																	"more path(s)!", true, true));
			return;
		}
		bool successful = true;

		// Save the vertex source first.
		std::ofstream vertFile(this->vertPath, std::ios::out | std::ios::trunc);

		if (vertFile.is_open())
		{
			vertFile << this->vertSource;
			vertFile.close();
		}
		else
		{
			logs->logMessage(LogMessage("Failed to save the vertex source.",
																	true, true));
			successful = false;
		}

		// Save the fragment source second.
		std::ofstream fragFile(this->fragPath, std::ios::out | std::ios::trunc);

		if (fragFile.is_open())
		{
			fragFile << this->fragSource;
			fragFile.close();
		}
		else
		{
			logs->logMessage(LogMessage("Failed to save the fragment source.",
																	true, true));
			successful = false;
		}

		if (successful)
			logs->logMessage(LogMessage("Successfully saved the shader.",
																	true, true));
	}

	// Saves the vertex and fragment shader source code to a new text file.
	void
	Shader::saveSourceToFiles(const std::string &vertPath,
														const std::string &fragPath)
	{
		Logger* logs = Logger::getInstance();

		if (vertPath == "" || fragPath == "")
		{
			logs->logMessage(LogMessage("Could not save shader file, missing one or "
																	"more paths!", true, true));
			return;
		}
		bool successful = true;

		// Save the vertex source first.
		std::ofstream vertFile(vertPath, std::ios::out | std::ios::trunc);

		if (vertFile.is_open())
		{
			vertFile << this->vertSource;
			vertFile.close();
		}
		else
		{
			logs->logMessage(LogMessage("Failed to save vertex source.",
																	true, true));
			successful = false;
		}

		// Save the fragment source second.
		std::ofstream fragFile(fragPath, std::ios::out | std::ios::trunc);

		if (fragFile.is_open())
		{
			fragFile << this->fragSource;
			fragFile.close();
		}
		else
		{
			logs->logMessage(LogMessage("Failed to save fragment source.",
																	true, true));
			successful = false;
		}

		if (successful)
			logs->logMessage(LogMessage("Successfully saved the shader files.",
																	true, true));
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
	Shader::addUniformSampler(const char* uniformName, GLuint texID)
	{
		this->bind();
		GLuint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform1i(uniLoc, texID);
	}

	// Vertex attribute setters.
	void
	Shader::addAtribute(const char* attribName, AttribType type,
											GLboolean normalized, unsigned size, unsigned stride)
	{
		this->bind();
		GLuint attribPos = glGetAttribLocation(this->progID, attribName);
		// Cast to long first to avoid a compiler warning on 64 bit systems.
		glVertexAttribPointer(attribPos, static_cast<GLint>(type), GL_FLOAT,
													normalized, size, (void*) (unsigned long) stride);
		glEnableVertexAttribArray(attribPos);
	}

	// Debug method to dump the shader program.
	std::string
	Shader::dumpProgram()
	{
		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		int uniforms, attributes, shaders;

		std::string output = "";

		if (!glIsProgram(this->progID))
		{
			output += "Not a valid shader program!";
			return output;
		}

		glGetProgramiv(this->progID, GL_ATTACHED_SHADERS, &shaders);
		output += "Number of attached shaders: ";
		output += std::to_string(shaders);
		output += "\n";

		glGetProgramiv(this->progID, GL_ACTIVE_UNIFORMS, &uniforms);
		output += "Number of active uniforms: ";
		output += std::to_string(uniforms);
		output += "\n";

		for (unsigned i = 0; i < uniforms; i++)
		{
			glGetActiveUniform(this->progID, i, 256, &length, &size, &type, name);
			output += "    Name: ";
			output += name;
			output += " (";
			output += enumToString(type);
			output += ")";
			output += "\n";
		}

		glGetProgramiv(this->progID, GL_ACTIVE_ATTRIBUTES, &attributes);
		output += "Number of active attributes: ";
		output += std::to_string(attributes);
		output += "\n";

		for (unsigned i = 0; i < attributes; i++)
		{
			glGetActiveAttrib(this->progID, i, 256, &length, &size, &type, name);
			output += "    Name: ";
			output += name;
			output += " (";
			output += enumToString(type);
			output += ")";
			output += "\n";
		}

		return output;
	}

	// Convert an enum to a string to make shader validation and material
	// properties easier to do. TODO: Add more uniforms and attributes.
	std::string
	Shader::enumToString(GLenum sEnum)
	{
		switch (sEnum)
		{
			case GL_FLOAT: return "float";
			case GL_FLOAT_VEC2: return "vec2";
			case GL_FLOAT_VEC3: return "vec3";
			case GL_FLOAT_VEC4: return "vec4";
			case GL_FLOAT_MAT3: return "mat3";
			case GL_FLOAT_MAT4: return "mat4";
			case GL_SAMPLER_1D: return "sampler1D";
			case GL_SAMPLER_2D: return "sampler2D";
			case GL_SAMPLER_3D: return "sampler3D";
			case GL_SAMPLER_CUBE: return "samplerCube";

			default: return "????";
		}
	}

	UniformType
	Shader::enumToUniform(GLenum sEnum)
	{
		switch (sEnum)
		{
			case GL_FLOAT: return UniformType::Float;
			case GL_FLOAT_VEC2: return UniformType::Vec2;
			case GL_FLOAT_VEC3: return UniformType::Vec3;
			case GL_FLOAT_VEC4: return UniformType::Vec4;
			case GL_FLOAT_MAT3: return UniformType::Mat3;
			case GL_FLOAT_MAT4: return UniformType::Mat4;
			case GL_SAMPLER_1D: return UniformType::Sampler1D;
			case GL_SAMPLER_2D: return UniformType::Sampler2D;
			case GL_SAMPLER_3D: return UniformType::Sampler3D;
			case GL_SAMPLER_CUBE: return UniformType::SamplerCube;

			default: return UniformType::Unknown;
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
