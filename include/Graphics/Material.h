#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"

namespace SciRenderer
{
  // Type of the material.
  enum class MaterialType
  {
    PBR, Specular, Unknown
  };

  class Material
  {
  public:
    Material(MaterialType type = MaterialType::PBR);
    Material(MaterialType type, const std::vector<std::pair<std::string, Texture2D*>> &sampler2Ds);
    ~Material();

    // Attach a texture.
    void attachTexture(Texture2D* tex, const std::string &name);

    // Prepare for drawing.
    void configure();

    // Check to see if the material has a texture of a certain type.
    bool hasTexture(const std::string &name);

    // Get the shader type.
    MaterialType& getType() { return this->type; }

    // Get the shader program.
    Shader* getShader() { return this->program; }

    // Get the shader data.
    GLfloat& getFloat(const std::string &name)
    {
      auto loc = pairGet<GLfloat>(this->floats, name);
      return loc->second;
    };
    glm::vec2& getVec2(const std::string &name)
    {
      auto loc = pairGet<glm::vec2>(this->vec2s, name);
      return loc->second;
    };
    glm::vec3& getVec3(const std::string &name)
    {
      auto loc = pairGet<glm::vec3>(this->vec3s, name);
      return loc->second;
    };
    Texture2D* getTexture(const std::string &name);

    bool& useMultiples() { return this->useUMultiples; }
  private:
    MaterialType type;

    // The shader and shader data.
    Shader* program;
    std::vector<std::pair<std::string, GLfloat>> floats;
    std::vector<std::pair<std::string, glm::vec2>> vec2s;
    std::vector<std::pair<std::string, glm::vec3>> vec3s;
    std::vector<std::pair<std::string, Texture2D*>> sampler2Ds;

    bool useUMultiples;

    // Searching functions.
    template <typename T>
    bool pairSearch(const std::vector<std::pair<std::string, T>> &list, const std::string &name)
    {
      auto loc = std::find_if(list.begin(), list.end(),
                              [&name](const std::pair<std::string, T> &pair)
      {
        return pair.first == name;
      });

      return loc != list.end();
    }

    template <typename T>
    std::vector<std::pair<std::string, T>>::iterator
    pairGet(std::vector<std::pair<std::string, T>> &list, const std::string &name)
    {
      auto loc = std::find_if(list.begin(), list.end(),
                              [&name](const std::pair<std::string, T> &pair)
      {
        return pair.first == name;
      });

      return loc;
    }
  };
}
