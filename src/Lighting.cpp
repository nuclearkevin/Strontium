#include "Lighting.h"

using namespace SciRenderer;

LightController::LightController(const char* vertPath, const char* fragPath,
                                 const char* lightMeshPath)
  : ambient(glm::vec3(0.0f, 0.0f, 0.0f))
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
}

LightController::~LightController()
{
  delete this->lightProgram;
  delete this->lightMesh;
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
  // This is a janky implementation. TODO: use uniform buffer objects to make
  // this faster.
  lightingShader->bind();
  lightingShader->addUniformVector("ambientColour", this->ambient);
  lightingShader->addUniformVector("camera.position", camera->getCamPos());
  lightingShader->addUniformVector("camera.viewDir", camera->getCamFront());

  // Set the uniform lights.
  if (this->uniformLights.size() < 9)
    lightingShader->addUniformUInt("numULights", this->uniformLights.size());
  else
    lightingShader->addUniformUInt("numULights", 8);
  for (unsigned i = 0; i < this->uniformLights.size(); i++)
  {
    // Guard so the maximum number of uniform lights (8) isn't exceeded.
    if (i == 8)
      break;
    lightingShader->addUniformVector((std::string("uLight[") + std::to_string(i)
        + std::string("].colour")).c_str(), uniformLights[i].colour);
    lightingShader->addUniformVector((std::string("uLight[") + std::to_string(i)
        + std::string("].direction")).c_str(), uniformLights[i].direction);
    lightingShader->addUniformFloat((std::string("uLight[") + std::to_string(i)
        + std::string("].intensity")).c_str(), uniformLights[i].intensity);
    lightingShader->addUniformVector((std::string("uLight[") + std::to_string(i)
        + std::string("].mat.diffuse")).c_str(), uniformLights[i].mat.diffuse);
    lightingShader->addUniformVector((std::string("uLight[") + std::to_string(i)
        + std::string("].mat.specular")).c_str(), uniformLights[i].mat.specular);
    lightingShader->addUniformFloat((std::string("uLight[") + std::to_string(i)
        + std::string("].mat.shininess")).c_str(), uniformLights[i].mat.shininess);
  }
  // Set the point lights.
  if (this->pointLights.size() < 9)
    lightingShader->addUniformUInt("numPLights", this->pointLights.size());
  else
    lightingShader->addUniformUInt("numPLights", 8);
  for (unsigned i = 0; i < this->pointLights.size(); i++)
  {
    // Guard so the maximum number of point lights (8) isn't exceeded.
    if (i == 8)
      break;
    lightingShader->addUniformVector((std::string("pLight[") + std::to_string(i)
        + std::string("].colour")).c_str(), pointLights[i].colour);
    lightingShader->addUniformVector((std::string("pLight[") + std::to_string(i)
        + std::string("].position")).c_str(), pointLights[i].position);
    lightingShader->addUniformFloat((std::string("pLight[") + std::to_string(i)
        + std::string("].intensity")).c_str(), pointLights[i].intensity);
    lightingShader->addUniformVector((std::string("pLight[") + std::to_string(i)
        + std::string("].mat.diffuse")).c_str(), pointLights[i].mat.diffuse);
    lightingShader->addUniformVector((std::string("pLight[") + std::to_string(i)
        + std::string("].mat.specular")).c_str(), pointLights[i].mat.specular);
    lightingShader->addUniformVector((std::string("pLight[") + std::to_string(i)
        + std::string("].mat.attenuation")).c_str(), pointLights[i].mat.attenuation);
    lightingShader->addUniformFloat((std::string("pLight[") + std::to_string(i)
        + std::string("].mat.shininess")).c_str(), pointLights[i].mat.shininess);
  }
  // Set the spot lights.
  if (this->spotLights.size() < 9)
    lightingShader->addUniformUInt("numSLights", this->spotLights.size());
  else
    lightingShader->addUniformUInt("numSLights", 8);
  for (unsigned i = 0; i < this->spotLights.size(); i++)
  {
    // Guard so the maximum number of spot lights (8) isn't exceeded.
    if (i == 8)
      break;
    lightingShader->addUniformVector((std::string("sLight[") + std::to_string(i)
        + std::string("].colour")).c_str(), spotLights[i].colour);
    lightingShader->addUniformVector((std::string("sLight[") + std::to_string(i)
        + std::string("].position")).c_str(), spotLights[i].position);
    lightingShader->addUniformVector((std::string("sLight[") + std::to_string(i)
        + std::string("].direction")).c_str(), spotLights[i].direction);
    lightingShader->addUniformFloat((std::string("sLight[") + std::to_string(i)
        + std::string("].intensity")).c_str(), spotLights[i].intensity);
    lightingShader->addUniformFloat((std::string("sLight[") + std::to_string(i)
        + std::string("].cosTheta")).c_str(), spotLights[i].innerCutOff);
    lightingShader->addUniformFloat((std::string("sLight[") + std::to_string(i)
        + std::string("].cosGamma")).c_str(), spotLights[i].outerCutOff);
    lightingShader->addUniformVector((std::string("sLight[") + std::to_string(i)
        + std::string("].mat.diffuse")).c_str(), spotLights[i].mat.diffuse);
    lightingShader->addUniformVector((std::string("sLight[") + std::to_string(i)
        + std::string("].mat.specular")).c_str(), spotLights[i].mat.specular);
    lightingShader->addUniformVector((std::string("sLight[") + std::to_string(i)
        + std::string("].mat.attenuation")).c_str(), spotLights[i].mat.attenuation);
    lightingShader->addUniformFloat((std::string("sLight[") + std::to_string(i)
        + std::string("].mat.shininess")).c_str(), spotLights[i].mat.shininess);
  }
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

// Getters.
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
