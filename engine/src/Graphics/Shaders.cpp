#include "Graphics/Shaders.h"

// Project includes.
#include "Core/Logs.h"
#include "Serialization/YamlSerialization.h"

// OpenGL includes.
#include "glad/glad.h"

// YAML includes.
#include "yaml-cpp/yaml.h"

namespace Strontium
{
  std::string
  Shader::shaderStageToString(const ShaderStage &stage)
  {
    switch (stage)
    {
      case ShaderStage::Vertex: return "Vertex";
      case ShaderStage::TessControl: return "Tesselation Control";
      case ShaderStage::TessEval: return "Tesselation Evaluation";
      case ShaderStage::Geometry: return "Geometry";
      case ShaderStage::Fragment: return "Fragment";
      case ShaderStage::Compute: return "Compute";
      default: return "Unknown Stage";
    }
  }

  void
  Shader::memoryBarrier(const MemoryBarrierType &type)
  {
    glMemoryBarrier(static_cast<GLenum>(type));
  }

  Shader::Shader()
  {
    this->progID = glCreateProgram();
  }

  Shader::Shader(const std::string &filepath)
  {
    this->progID = glCreateProgram();

    // Load the shader source code from disk into the map.
    this->loadFile(filepath);

    // Compile each source saved in the map.
    std::vector<uint> shaderBindaries;
    for (auto&& [stage, source] : this->shaderSources)
      shaderBindaries.emplace_back(this->compileStage(stage, source));

    // Link the individual shader programs, deletes after linking is completed.
    this->linkProgram(shaderBindaries);
  }

  Shader::~Shader()
  {
    glDeleteProgram(this->progID);
  }

  void
  Shader::rebuild()
  {
    glDeleteProgram(this->progID);
    this->progID = glCreateProgram();

    this->shaderSources.clear();

    // Load the shader source code from disk into the map.
    this->loadFile(this->shaderPath);

    // Compile each source saved in the map.
    std::vector<uint> shaderBindaries;
    for (auto&& [stage, source] : this->shaderSources)
      shaderBindaries.emplace_back(this->compileStage(stage, source));

    // Link the individual shader programs, deletes after linking is completed.
    this->linkProgram(shaderBindaries);
  }

  void
  Shader::loadFile(const std::string &filepath)
  {
    // Common shader parameters. Declare with: #type common
    std::string commonBlock = "";

    // Garbage needed for parsing the shader file.
    std::string line;
    std::string currentBlock = "";
    bool readingCommon = false;
    ShaderStage currentStage = ShaderStage::Unknown;

    std::unordered_map<ShaderStage, std::string> commonlessSource;

    std::ifstream sourceFile(filepath);
    if (sourceFile.is_open())
    {
      this->shaderPath = filepath;

      while (std::getline(sourceFile, line))
      {
        // Detect shader stage.
        if (line.substr(0, 6) == "#type ")
        {
          // Save the buffer string to a map. This doesn't have the common block
          // added in.
          if (currentStage != ShaderStage::Unknown && !readingCommon)
          {
            commonlessSource.emplace(currentStage, currentBlock);
            currentBlock.clear();
          }

          if (line.find("vertex") != std::string::npos)
          {
            currentStage = ShaderStage::Vertex;
            readingCommon = false;
          }
          else if (line.find("tess_control") != std::string::npos)
          {
            currentStage = ShaderStage::TessControl;
            readingCommon = false;
          }
          else if (line.find("tess_eval") != std::string::npos)
          {
            currentStage = ShaderStage::TessEval;
            readingCommon = false;
          }
          else if (line.find("geometry") != std::string::npos)
          {
            currentStage = ShaderStage::Geometry;
            readingCommon = false;
          }
          else if (line.find("fragment") != std::string::npos)
          {
            currentStage = ShaderStage::Fragment;
            readingCommon = false;
          }
          else if (line.find("compute") != std::string::npos)
          {
            currentStage = ShaderStage::Compute;
            readingCommon = false;
          }
          else if (line.find("common") != std::string::npos)
          {
            currentStage = ShaderStage::Unknown;
            readingCommon = true;
          }
          else
          {
            // Log the shader parse log.
            std::string message = "Unknown shader stage " + filepath + ".";
            Logs::log(message);
            currentStage = ShaderStage::Unknown;
            readingCommon = false;
          }
        }
        else
        {
          // Append the line to the source code.
          if (readingCommon)
            commonBlock += line + "\n";

          if (currentStage != ShaderStage::Unknown)
            currentBlock += line + "\n";
        }
      }

      // Add in the final block.
      if (currentStage != ShaderStage::Unknown)
        commonlessSource.emplace(currentStage, currentBlock);

      // Save the resulting sources with the common clock appended at the beginning.
      for (auto&& [stage, source] : commonlessSource)
        this->shaderSources.emplace(stage, commonBlock + source);
    }
    else
    {
      std::string message = "Failed to open shader file at " + filepath + ".";
      Logs::log(message);
    }
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
  Shader::launchCompute(uint globalX, uint globalY, uint globalZ)
  {
    assert(("Shader does not have a compute stage. ", this->shaderSources.count(ShaderStage::Compute)));
    glUseProgram(this->progID);
    glDispatchCompute(globalX, globalY, globalZ);
  }

  uint
  Shader::compileStage(const ShaderStage &stage, const std::string &stageSource)
  {
    char* source = (char*) stageSource.c_str();

    // Compile the source.
    uint shaderID = glCreateShader(static_cast<GLenum>(stage));
    glShaderSource(shaderID, 1, (const  GLchar**) &source, nullptr);
    glCompileShader(shaderID);

    // Check to see if the compilation succeeded.
    int result;
    char* buffer;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE)
    {
      glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &result);
      buffer = new char[result];
      glGetShaderInfoLog(shaderID, result, 0, buffer);

      std::string message = "Shader compiler error at: "
						  + shaderStageToString(stage) + "\n"
						  + std::string(buffer);
      Logs::log(message);

      delete buffer;
    }

    return shaderID;
  }

  void
  Shader::linkProgram(const std::vector<uint> &binaries)
  {
    // Attach separate shader binaries.
    for (auto& shaderID : binaries)
      glAttachShader(this->progID, shaderID);

    // Link the shader program.
    glLinkProgram(this->progID);

    // Check to see if the link succeeded.
    int result;
    char* buffer;
		glGetProgramiv(this->progID, GL_LINK_STATUS, &result);
    if (result != GL_TRUE)
		{
      glGetProgramiv(this->progID, GL_INFO_LOG_LENGTH, &result);
      buffer = new char[result];
			glGetProgramInfoLog(this->progID, result, 0, buffer);

      std::string message = "Shader link error:\n"
							+ std::string(buffer);
      Logs::log(message);

      delete buffer;
    }

    // Delete the shader binaries now, no longer needed as they are linked in a
    // program.
    for (auto& shaderID : binaries)
      glDeleteShader(shaderID);
  }

  // Push uniform data.
	void
	Shader::addUniformMatrix(const char* uniformName, const glm::mat4 &matrix,
													  bool transpose)
	{
		GLboolean glTranspose = static_cast<GLboolean>(transpose);
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniformMatrix4fv(uniLoc, 1, glTranspose, glm::value_ptr(matrix));
	}

	void
	Shader::addUniformMatrix(const char* uniformName, const glm::mat3 &matrix,
													  bool transpose)
	{
		GLboolean glTranspose = static_cast<GLboolean>(transpose);
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniformMatrix3fv(uniLoc, 1, glTranspose, glm::value_ptr(matrix));
	}

	void
	Shader::addUniformMatrix(const char* uniformName, const glm::mat2 &matrix,
													  bool transpose)
	{
		GLboolean glTranspose = static_cast<GLboolean>(transpose);
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniformMatrix2fv(uniLoc, 1, glTranspose, glm::value_ptr(matrix));
	}

	void
	Shader::addUniformVector(const char* uniformName, const glm::vec4 &vector)
	{
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform4f(uniLoc, vector[0], vector[1], vector[2], vector[3]);
	}

	void
	Shader::addUniformVector(const char* uniformName, const glm::vec3 &vector)
	{
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform3f(uniLoc, vector[0], vector[1], vector[2]);
	}

	void
	Shader::addUniformVector(const char* uniformName, const glm::vec2 &vector)
	{
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform2f(uniLoc, vector[0], vector[1]);
	}

	void
	Shader::addUniformFloat(const char* uniformName, float value)
	{
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform1f(uniLoc, value);
	}

	void
	Shader::addUniformInt(const char* uniformName, int value)
	{
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform1i(uniLoc, value);
	}

	void
	Shader::addUniformUInt(const char* uniformName, uint value)
	{
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform1ui(uniLoc, value);
	}

	void
	Shader::addUniformSampler(const char* uniformName, uint texID)
	{
		glUseProgram(this->progID);
		uint uniLoc = glGetUniformLocation(this->progID, uniformName);
		glUniform1i(uniLoc, texID);
	}

    namespace ShaderCache
    {
      std::unordered_map<std::string, Shader> shaderCache;

      void 
      init(const std::string& filepath)
      {
        shaderCache = std::unordered_map<std::string, Shader>();

        YAML::Node data = YAML::LoadFile(filepath);
        if (!data["ShaderCache"])
        {
          assert("Shader cache not found");
        }
        else
        {
          auto shaderList = data["ShaderCache"];
          for (auto shader : shaderList)
          {
            if (shader["Handle"] && shader["Filepath"])
            {
              std::string handle = shader["Handle"].as<std::string>();
              std::string path = shader["Filepath"].as<std::string>();
              Logs::log("Compiling " + handle);
              shaderCache.emplace(handle, path);
            }
          }
        } 
      }

      Shader*
      getShader(const std::string& shaderHandle)
      {
        return &(shaderCache.at(shaderHandle));
      }

      std::unordered_map<std::string, Shader>::iterator 
      begin()
      {
        return shaderCache.begin();
      }
      std::unordered_map<std::string, Shader>::iterator 
      end()
      {
        return shaderCache.end();
      }
    }
}
