#include "Lighting.h"

using namespace SciRenderer;

LightController::LightController(const char* vertPath, const char* fragPath, const char* lightMeshPath)
{
  // Generate the light shader.
  this->lightProgram = new Shader(vertPath, fragPath);
  // Load the light mesh.
  this->lightMesh = new Mesh();
  this->lightMesh->loadOBJFile(lightMeshPath);
  this->lightMesh->generateVAO(this->lightProgram);
}

// Add lights to be applied.
void
LightController::addLight(const UniformLight& light)
{
  this->uniformLights.push_back(light);
}

void
LightController::addLight(const PointLight& light, GLfloat scaleFactor)
{
  this->pointLights.push_back(light);
  this->pointLightModels.emplace_back(glm::mat4(1.0f));
  unsigned insertIndex = this->pointLightModels.size() - 1;
  this->pointLightModels[insertIndex] =
        glm::scale(this->pointLightModels[insertIndex],
                   glm::vec3(scaleFactor, scaleFactor, scaleFactor));
  this->pointLightModels[insertIndex] =
        glm::translate(this->pointLightModels[insertIndex],
                       glm::vec3(light.position.x / scaleFactor,
                                 light.position.y / scaleFactor,
                                 light.position.z / scaleFactor));
}

void
LightController::addLight(const SpotLight& light, GLfloat scaleFactor)
{
  this->spotLights.push_back(light);
  this->spotLightModels.emplace_back(glm::mat4(1.0f));
  unsigned insertIndex = this->spotLightModels.size() - 1;
  this->spotLightModels[insertIndex] =
        glm::scale(this->spotLightModels[insertIndex],
                   glm::vec3(scaleFactor, scaleFactor, scaleFactor));
  this->spotLightModels[insertIndex] =
        glm::translate(this->spotLightModels[insertIndex],
                       glm::vec3(light.position.x / scaleFactor,
                                 light.position.y / scaleFactor,
                                 light.position.z / scaleFactor));
}

// Set the light uniforms in the lighting shader for the next frame.
void
LightController::setLighting(Shader* lightingShader)
{
  lightingShader->bind();
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
    glm::mat4 mVP = camera->getProjMatrix() * camera->getViewMatrix()
                    * this->pointLightModels[i];
    this->lightProgram->addUniformMatrix("mVP", mVP, GL_FALSE);
    renderer->draw(this->lightMesh->getVAO(), this->lightProgram);
  }

  // Draw the spotlight meshes.
  for (unsigned i = 0; i < this->spotLights.size(); i++)
  {
    if (i == 8)
      break;
    this->lightProgram->addUniformVector("colour", this->spotLights[i].colour);
    glm::mat4 mVP = camera->getProjMatrix() * camera->getViewMatrix()
                    * this->spotLightModels[i];
    this->lightProgram->addUniformMatrix("mVP", mVP, GL_FALSE);
    renderer->draw(this->lightMesh->getVAO(), this->lightProgram);
  }
}

// Delete a light. TODO
void
LightController::deleteLight(LightType type, GLuint lightID)
{

}

// Getters.
UniformLight*
LightController::getULight(GLuint uLightID)
{
  if (this->uniformLights.size() < uLightID)
    return &this->uniformLights[uLightID];
  else
    return nullptr;
}

PointLight*
LightController::getPLight(GLuint pLightID)
{
  if (this->pointLights.size() < pLightID)
    return &this->pointLights[pLightID];
  else
    return nullptr;
}

SpotLight*
LightController::getSLightID(GLuint sLightID)
{
  if (this->spotLights.size() < sLightID)
    return &this->spotLights[sLightID];
  else
    return nullptr;
}
