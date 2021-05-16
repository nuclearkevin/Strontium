#include "Graphics/Compute.h"

namespace SciRenderer
{
  ComputeShader::ComputeShader(const std::string &filepath)
  {
    this->buildShader(GL_COMPUTE_SHADER, filepath.c_str());
    this->buildProgram();
  }

  ComputeShader::~ComputeShader()
  {
    glDeleteProgram(this->progID);
		glDeleteShader(this->computeID);
    this->unbind();
  }

  // Bind/unbind the compute program.
  void
  ComputeShader::bind()
  {
    glUseProgram(this->progID);
  }

  void
  ComputeShader::unbind()
  {
    glUseProgram(0);
  }

  void
  ComputeShader::launchCompute(const glm::ivec3 &computeUnits)
  {
    this->bind();
    glDispatchCompute(computeUnits.x, computeUnits.y, computeUnits.z);
  }

  // Build the compute shader.
  void
	ComputeShader::buildShader(int type, const char* filename)
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
			case GL_COMPUTE_SHADER:
				this->computeID = shaderID;
				break;
			default:
				printf("Shader type unknown!\n");
				break;
		}
	}

  // Build the compute program.
  void
  ComputeShader::buildProgram()
  {
    this->progID = glCreateProgram();
    glAttachShader(this->progID, this->computeID);

    int result;
    glLinkProgram(this->progID);
		glGetProgramiv(this->progID, GL_LINK_STATUS, &result);

    char* buffer;
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

  // Read in the shader source code.
  char*
  ComputeShader::readShaderFile(const char* filename)
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
