#version 440
/*
 * A fragment shader to compute the diffuse lighting integral through
 * convolution.
*/

#define PI 3.141592654

layout(location = 0) out vec4 fragColour;

in vec3 fPosition;

uniform samplerCube environmentMap;

void main()
{
  // The normal is the same as the fragments worldspace position (as the view
  // transform took care of that in the vertex shader).
  vec3 fNormal = normalize(fPosition);

  // Other hemispherical directions required for the integral.
  vec3 fUp     = vec3(0.0, 1.0, 0.0);
  vec3 fRight  = cross(fUp, fNormal);
  fUp          = cross(fNormal, fRight);

  float sampleDelta = 0.025;
  float nrSamples = 0.0;
  vec3 irradiance = vec3(0.0);

  for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
  {
      for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
      {
        // spherical to cartesian (in tangent space)
        vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
        // tangent space to world
        vec3 sampleVec = tangentSample.x * fRight + tangentSample.y * fUp + tangentSample.z * fNormal;
        irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
        nrSamples++;
      }
  }

  irradiance = PI * irradiance * (1.0 / float(nrSamples));

  fragColour = vec4(irradiance, 1.0);
}
