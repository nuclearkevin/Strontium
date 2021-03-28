#include "Lighting.h"

using namespace SciRenderer;

LightController::LightController(const char* vertPath, const char* fragPath,
                                 const char* lightMeshPath)
  : ambient(glm::vec3(1.0f, 1.0f, 1.0f))
  , uLightCounter(0)
  , pLightCounter(0)
  , sLightCounter(0)
{
  // Generate the light shader.
  this->lightProgram = new Shader(vertPath, fragPath);
  // Load the light mesh.
  this->lightMesh = new Mesh();
  this->lightMesh->loadOBJFile(lightMeshPath);
  this->lightMesh->generateVAO(this->lightProgram);

  // Generate 3 uniform buffers for the lights.
  this->ulightBuffer = new UniformBuffer(8 * sizeof(ShaderUL), STATIC);
  this->ulightBuffer->bindToPoint(1);
  this->plightBuffer = new UniformBuffer(8 * sizeof(ShaderPL), STATIC);
  this->plightBuffer->bindToPoint(2);
  this->slightBuffer = new UniformBuffer(8 * sizeof(ShaderSL), STATIC);
  this->slightBuffer->bindToPoint(3);
}

LightController::~LightController()
{
  delete this->lightProgram;
  delete this->lightMesh;
  delete this->ulightBuffer;
  delete this->plightBuffer;
  delete this->slightBuffer;
}

glm::vec3*
LightController::getAmbient()
{
  return &this->ambient;
}

// Add lights to be applied.
void
LightController::addLight(const UniformLight& light)
{
  if (this->uniformLights.size() + 1 > 8)
    return;
  this->uniformLights.push_back(light);
  unsigned insert = this->uniformLights.size() - 1;
  this->uniformLights[insert].direction = this->uniformLights[insert].direction;
  this->uniformLights[insert].name = std::string("UL ") + std::to_string(insert + 1);
  this->uniformLights[insert].lightID = this->uLightCounter;

  this->uLightCounter++;
  this->setGuiLabel(UNIFORM);
}

void
LightController::addLight(const PointLight& light, GLfloat scaleFactor)
{
  if (this->pointLights.size() + 1 > 8)
    return;
  this->pointLights.push_back(light);
  unsigned insert = this->pointLights.size() - 1;
  this->pointLights[insert].meshScale = scaleFactor;
  this->pointLights[insert].name = std::string("PL ") + std::to_string(insert + 1);
  this->pointLights[insert].lightID = this->pLightCounter;

  this->pLightCounter++;
  this->setGuiLabel(POINT);
}

void
LightController::addLight(const SpotLight& light, GLfloat scaleFactor)
{
  if (this->spotLights.size() + 1 > 8)
    return;
  this->spotLights.push_back(light);
  unsigned insert = this->spotLights.size() - 1;
  this->spotLights[insert].direction = this->spotLights[insert].direction;
  this->spotLights[insert].meshScale = scaleFactor;
  this->spotLights[insert].name = std::string("SL ") + std::to_string(insert + 1);
  this->spotLights[insert].lightID = this->sLightCounter;

  this->sLightCounter++;
  this->setGuiLabel(SPOT);
}

// Set the light uniforms in the lighting shader for the next frame.
void
LightController::setLighting(Shader* lightingShader, Camera* camera)
{
  lightingShader->bind();
  lightingShader->addUniformVector("ambientColour", this->ambient);
  lightingShader->addUniformVector("camera.position", camera->getCamPos());
  lightingShader->addUniformVector("camera.viewDir", camera->getCamFront());

  // Fixed-size arrays for each type of light, since the shader can only support
  // 8 at maximum.
  ShaderUL ulCollection[8];
  ShaderPL plCollection[8];
  ShaderSL slCollection[8];

  // Set the uniform lights.
  if (this->uniformLights.size() < 9)
    lightingShader->addUniformUInt("numULights", this->uniformLights.size());
  else
    lightingShader->addUniformUInt("numULights", 8);

  for (unsigned i = 0; i < 8; i++)
  {
    if (i + 1 <= this->uniformLights.size())
    {
      ulCollection[i].colour = glm::vec4(this->uniformLights[i].colour, 1.0f);
      ulCollection[i].direction = glm::vec4(this->uniformLights[i].direction, 1.0f);
      ulCollection[i].diffuse = this->uniformLights[i].mat.diffuse;
      ulCollection[i].specular = this->uniformLights[i].mat.specular;
      ulCollection[i].attenuation = this->uniformLights[i].mat.attenuation;
      ulCollection[i].shininess = this->uniformLights[i].mat.shininess;
      ulCollection[i].intensity = this->uniformLights[i].intensity;
    }
    else
      break;
  }

  // Set the point lights.
  if (this->pointLights.size() < 9)
    lightingShader->addUniformUInt("numPLights", this->pointLights.size());
  else
    lightingShader->addUniformUInt("numPLights", 8);

  for (unsigned i = 0; i < 8; i++)
  {
    if (i + 1 <= this->pointLights.size())
    {
      plCollection[i].colour = glm::vec4(this->pointLights[i].colour, 1.0f);
      plCollection[i].position = glm::vec4(this->pointLights[i].position, 1.0f);
      plCollection[i].intensity = this->pointLights[i].intensity;
      plCollection[i].diffuse = this->pointLights[i].mat.diffuse;
      plCollection[i].specular = this->pointLights[i].mat.specular;
      plCollection[i].attenuation = this->pointLights[i].mat.attenuation;
      plCollection[i].shininess = this->pointLights[i].mat.shininess;
      plCollection[i].intensity = this->pointLights[i].intensity;
    }
    else
      break;
  }

  // Set the spotlights.
  if (this->spotLights.size() < 9)
    lightingShader->addUniformUInt("numSLights", this->spotLights.size());
  else
    lightingShader->addUniformUInt("numSLights", 8);

  for (unsigned i = 0; i < 8; i++)
  {
    if (i + 1 <= this->spotLights.size())
    {
      slCollection[i].colour = glm::vec4(this->spotLights[i].colour, 1.0f);
      slCollection[i].position = glm::vec4(this->spotLights[i].position, 1.0f);
      slCollection[i].direction = glm::vec4(this->spotLights[i].direction, 1.0f);
      slCollection[i].intensity = this->spotLights[i].intensity;
      slCollection[i].cosTheta = this->spotLights[i].innerCutOff;
      slCollection[i].cosGamma = this->spotLights[i].outerCutOff;
      slCollection[i].diffuse = this->spotLights[i].mat.diffuse;
      slCollection[i].specular = this->spotLights[i].mat.specular;
      slCollection[i].attenuation = this->spotLights[i].mat.attenuation;
      slCollection[i].shininess = this->spotLights[i].mat.shininess;
      slCollection[i].intensity = this->spotLights[i].intensity;
    }
    else
      break;
  }

  // Stream the data to the buffer and bind it to position 1->3.
  this->ulightBuffer->setData(0, sizeof(ulCollection), &ulCollection);
  this->plightBuffer->setData(0, sizeof(plCollection), &plCollection);
  this->slightBuffer->setData(0, sizeof(slCollection), &slCollection);
  this->ulightBuffer->bindToPoint(1);
  this->plightBuffer->bindToPoint(2);
  this->slightBuffer->bindToPoint(3);
  lightingShader->unbind();
}

// Draw the light meshes.
void
LightController::drawLightMeshes(Renderer* renderer, Camera* camera)
{
  // Draw the point light meshes.
  for (unsigned i = 0; i < this->pointLights.size(); i++)
  {
    if (i == 8)
      break;
    this->lightProgram->addUniformVector("colour", this->pointLights[i].colour);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(this->pointLights[i].position));
    model = glm::scale(model, glm::vec3(this->pointLights[i].meshScale,
                                        this->pointLights[i].meshScale,
                                        this->pointLights[i].meshScale));
    glm::mat4 mVP = camera->getProjMatrix() * camera->getViewMatrix()
                    * model;
    this->lightProgram->addUniformMatrix("mVP", mVP, GL_FALSE);
    renderer->draw(this->lightMesh->getVAO(), this->lightProgram);
  }

  // Draw the spotlight meshes.
  for (unsigned i = 0; i < this->spotLights.size(); i++)
  {
    if (i == 8)
      break;
    this->lightProgram->addUniformVector("colour", this->spotLights[i].colour);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(this->spotLights[i].position));
    model = glm::scale(model, glm::vec3(this->spotLights[i].meshScale,
                                        this->spotLights[i].meshScale,
                                        this->spotLights[i].meshScale));
    glm::mat4 mVP = camera->getProjMatrix() * camera->getViewMatrix()
                    * model;
    this->lightProgram->addUniformMatrix("mVP", mVP, GL_FALSE);
    renderer->draw(this->lightMesh->getVAO(), this->lightProgram);
  }
}

// Delete a light.
void
LightController::deleteLight(LightType type, GLuint lightID)
{
  switch (type)
  {
    case UNIFORM:
      for (unsigned i = 0; i < this->uniformLights.size(); i++)
      {
        if (this->uniformLights[i].lightID == lightID)
        {
          this->uniformLights.erase(this->uniformLights.begin() + i);
          return;
        }
      }
    case POINT:
      for (unsigned i = 0; i < this->pointLights.size(); i++)
      {
        if (this->pointLights[i].lightID == lightID)
        {
          this->pointLights.erase(this->pointLights.begin() + i);
          return;
        }
      }
    case SPOT:
      for (unsigned i = 0; i < this->spotLights.size(); i++)
      {
        if (this->spotLights[i].lightID == lightID)
        {
          this->spotLights.erase(this->spotLights.begin() + i);
          return;
        }
      }
    default:
      return;
  }
  this->setGuiLabel(type);
}

UniformLight*
LightController::getULight(GLuint uLightID)
{
  if (uLightID < this->uniformLights.size())
    return &this->uniformLights[uLightID];
  else
    return nullptr;
}

PointLight*
LightController::getPLight(GLuint pLightID)
{
  if (pLightID < this->pointLights.size())
    return &this->pointLights[pLightID];
  else
    return nullptr;
}

SpotLight*
LightController::getSLight(GLuint sLightID)
{
  if (sLightID < this->spotLights.size())
    return &this->spotLights[sLightID];
  else
    return nullptr;
}

GLuint
LightController::getNumLights(LightType type)
{
  switch (type)
  {
    case UNIFORM:
      return this->uniformLights.size();
    case POINT:
      return this->pointLights.size();
    case SPOT:
      return this->spotLights.size();
    case ALL:
      return this->spotLights.size() + this->pointLights.size() + this->uniformLights.size();
    default:
      return 0;
  }
}

std::vector<std::string>&
LightController::getGuiLabel(LightType type)
{
  switch (type)
  {
    case UNIFORM:
      return this->uLGuiLabel;
    case POINT:
      return this->pLGuiLabel;
    case SPOT:
      return this->sLGuiLabel;
    default:
      return this->allLightGuiLabel;
  }
}

// Setup the GUI label for the lighting GUI.
void
LightController::setGuiLabel(LightType type)
{
  switch (type)
  {
    case UNIFORM:
      this->uLGuiLabel.clear();
      this->uLGuiLabel.reserve(this->getNumLights(type));
      this->uLGuiLabel.insert(this->uLGuiLabel.begin(), std::string("----"));
      for (unsigned i = 0; i < uniformLights.size(); i++)
      {
        this->uLGuiLabel.push_back(this->uniformLights[i].name);
      }
    case POINT:
      this->pLGuiLabel.clear();
      this->pLGuiLabel.reserve(this->getNumLights(type));
      this->pLGuiLabel.insert(this->pLGuiLabel.begin(), std::string("----"));
      for (unsigned i = 0; i < pointLights.size(); i++)
      {
        this->pLGuiLabel.push_back(this->pointLights[i].name);
      }
    case SPOT:
      this->sLGuiLabel.clear();
      this->sLGuiLabel.reserve(this->getNumLights(type));
      this->sLGuiLabel.insert(this->sLGuiLabel.begin(), std::string("----"));
      for (unsigned i = 0; i < spotLights.size(); i++)
      {
        this->sLGuiLabel.push_back(this->spotLights[i].name);
      }
    default:
      this->allLightGuiLabel.clear();
      this->allLightGuiLabel.reserve(this->getNumLights(ALL));
      this->allLightGuiLabel.insert(this->allLightGuiLabel.begin(), std::string("----"));
      for (unsigned i = 0; i < uniformLights.size(); i++)
      {
        this->allLightGuiLabel.push_back(this->uniformLights[i].name);
      }
      for (unsigned i = 0; i < pointLights.size(); i++)
      {
        this->allLightGuiLabel.push_back(this->pointLights[i].name);
      }
      for (unsigned i = 0; i < spotLights.size(); i++)
      {
        this->allLightGuiLabel.push_back(this->spotLights[i].name);
      }
  }
}
