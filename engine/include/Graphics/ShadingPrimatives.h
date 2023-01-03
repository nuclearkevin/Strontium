#pragma once

// A series of packed primatives for shading.
namespace Strontium
{
  using RendererDataHandle = int;

  struct Camera
  {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 invViewProj;

    glm::vec3 position;
    float fov;
    float near;
    float far;

    glm::vec3 front;

    Camera()
      : near(0.1f)
      , far(30.0f)
      , fov(glm::radians(90.0f))
      , view(1.0f)
      , projection(1.0f)
      , invViewProj(glm::inverse(projection * view))
      , position(0.0)
      , front(0.0f, 0.0f, -1.0f)
    { }
  };

  struct DirectionalLight
  {
    glm::vec4 colourIntensity;
    glm::vec4 directionSize;

    DirectionalLight()
      : directionSize(0.0f, 1.0f, 0.0f, 10.0f)
      , colourIntensity(1.0f)
    { }

    DirectionalLight(const glm::vec3 colour, float intensity, 
                     const glm::vec3 &direction, float size)
      : colourIntensity(colour, intensity)
      , directionSize(direction, size)
    { }
  };

  struct PointLight
  {
    glm::vec4 positionRadius;
    glm::vec4 colourIntensity;

    PointLight()
      : positionRadius(0.0f)
      , colourIntensity(1.0f, 1.0f, 1.0f, 0.0f)
    { }

    PointLight(const glm::vec4 &positionRadius, const glm::vec4 &colourIntensity)
      : positionRadius(positionRadius)
      , colourIntensity(colourIntensity)
    { }
  };

  struct SpotLight
  {
    glm::vec4 positionRange;
    glm::vec4 direction; // w is empty.
    glm::vec4 cullingSphere; // Sphere center (x, y, z) and radius (w).
    glm::vec4 colourIntensity;
    glm::vec4 cutOffs; // Cos(inner angle) (x), Cos(outer angle) (y). z and w are empty.

    SpotLight()
      : positionRange(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
      , direction(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      , cullingSphere(0.0f, 0.0f, 0.0f, 1.0f)
      , colourIntensity(glm::vec4(1.0f))
      , cutOffs(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
    { }

    SpotLight(const glm::vec4 &positionRange, const glm::vec4 &direction,
              const glm::vec4 &colourIntensity, const glm::vec4 &cutOffs)
      : positionRange(positionRange)
      , direction(direction)
      , cullingSphere(0.0f, 0.0f, 0.0f, 1.0f)
      , colourIntensity(colourIntensity)
      , cutOffs(cutOffs)
    { }
  };

  struct RectAreaLight
  {
    glm::vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
    // Points of the rectangular area light (x, y, z). 
    // points[0].w > 0 indicates the light is two-sided, one-sided otherwise.
    glm::vec4 points[4];

    RectAreaLight()
      : colourIntensity(0.0f)
      , points{ {0.0f, 1.0f, -1.0f, 1.0}, {0.0f, 1.0f, 1.0f, 1.0}, {0.0f, -1.0f, 1.0f, 1.0}, {0.0f, -1.0f, -1.0f, 1.0} }
    { }

    RectAreaLight(const glm::vec4 &colourIntensity)
      : colourIntensity(colourIntensity)
      , points{ {0.0f, 1.0f, -1.0f, 1.0}, {0.0f, 1.0f, 1.0f, 1.0}, {0.0f, -1.0f, 1.0f, 1.0}, {0.0f, -1.0f, -1.0f, 1.0} }
    { }

    RectAreaLight(const glm::vec4 &colourIntensity, const glm::vec4 &pos0, 
                  const glm::vec4 &pos1, const glm::vec4 &pos2, const glm::vec4 &pos3)
      : colourIntensity(colourIntensity)
      , points{ pos0, pos1, pos2, pos3 }
    { } 
  };

  // All falloffs are in km.
  struct Atmosphere
  {
    glm::vec4 rayleighScat; // Rayleigh scattering base (x, y, z) and height falloff (w).
    glm::vec4 rayleighAbs; // Rayleigh absorption base (x, y, z) and height falloff (w).
    glm::vec4 mieScat; // Mie scattering base (x, y, z) and height falloff (w).
    glm::vec4 mieAbs; // Mie absorption base (x, y, z) and height falloff (w).
    glm::vec4 ozoneAbs; // Ozone absorption base (x, y, z) and scale (w).
    glm::vec4 planetAlbedoRadius; //Planet albedo (x, y, z) and radius.
    glm::vec4 sunDirAtmRadius; // Sun direction (x, y, z) and atmosphere radius (w).
    glm::vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
    glm::vec4 viewPos; // View position relative to the planet center (x, y, z). w is unused.

    Atmosphere(const glm::vec4 &rayleighScat, 
               const glm::vec4 &rayleighAbs, 
               const glm::vec4 &mieScat, 
               const glm::vec4 &mieAbs, 
               const glm::vec4 &ozoneAbs, 
               const glm::vec4 &planetAlbedoRadius, 
               const glm::vec4 &sunDirAtmRadius, 
               const glm::vec4 &lightColourIntensity,
               const glm::vec4 &viewPos)
      : rayleighScat(rayleighScat)
      , rayleighAbs(rayleighAbs)
      , mieScat(mieScat)
      , mieAbs(mieAbs)
      , ozoneAbs(ozoneAbs)
      , planetAlbedoRadius(planetAlbedoRadius)
      , sunDirAtmRadius(sunDirAtmRadius)
      , lightColourIntensity(lightColourIntensity)
      , viewPos(viewPos)
    { }

    Atmosphere() = default;

    bool operator ==(const Atmosphere &other)
    {
      return this->rayleighScat == other.rayleighScat 
             && this->rayleighAbs == other.rayleighAbs 
             && this->mieScat == other.mieScat
             && this->mieAbs == other.mieAbs
             && this->ozoneAbs == other.ozoneAbs
             && this->planetAlbedoRadius == other.planetAlbedoRadius
             && this->sunDirAtmRadius == other.sunDirAtmRadius
             && this->lightColourIntensity == other.lightColourIntensity
             && this->viewPos == other.viewPos;
    }

    bool operator !=(const Atmosphere& other)
    {
      return !((*this) == other);
    }
  };

  struct DynamicIBL
  {
    float intensity;
    RendererDataHandle handle;
    RendererDataHandle attachedSkyAtmoHandle;

    DynamicIBL()
      : intensity(0.0f)
      , handle(-1)
      , attachedSkyAtmoHandle(-1)
    { }

    DynamicIBL(float intensity, RendererDataHandle handle, 
               RendererDataHandle attachedSkyAtmoHandle)
      : intensity(intensity)
      , handle(handle)
      , attachedSkyAtmoHandle(attachedSkyAtmoHandle)
    { }
  };

  struct OBBFogVolume
  {
    glm::vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
    glm::vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
    glm::mat4 invTransformMatrix; // Inverse model-space transform matrix.

    OBBFogVolume(float phase, float density, float absorption, 
                 const glm::vec3 &mieScattering, const glm::vec3 &emission, 
                 const glm::mat4 &transformMatrix)
      : mieScatteringPhase(density * mieScattering, phase)
      , emissionAbsorption(density * emission, density * absorption)
      , invTransformMatrix(glm::inverse(transformMatrix))
    { }
  };

  struct SphereFogVolume
  {
    glm::vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
    glm::vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
    glm::vec4 positionRadius; // Position (x, y, z) are radius (w).

    SphereFogVolume(float phase, float density, float absorption,
                    const glm::vec3& mieScattering, const glm::vec3& emission,
                    const glm::vec3& position, float radius)
                    : mieScatteringPhase(density * mieScattering, phase)
                    , emissionAbsorption(density * emission, density * absorption)
                    , positionRadius(position, radius)
    { }
  };
}
