#type compute
#version 460 core

#define MAX_NUM_LIGHTS 512
#define TILE_SIZE 16
#define PI 3.141592654

// We use 16 x 16 warps here to match the tile size for the tiled deferred implementation.
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct SpotLight
{
  vec4 positionRange; // Position (x, y, z), range (w).
  vec4 direction; // Spot light direction (x, y, z). w is empty.
  vec4 cullingSphere; // Sphere center (x, y, z) and radius (w).
  vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
  vec4 cutOffs; // Cos(inner angle) (x), Cos(outer angle) (y). z and w are empty.
};

layout(rgba16f, binding = 0) restrict uniform image3D inScatExt;

layout(binding = 0) uniform sampler3D scatExtinction;
layout(binding = 1) uniform sampler3D emissionPhase;
layout(binding = 2) uniform sampler2D noise; // Blue noise

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

// Temporal AA parameters. TODO: jittered camera matrices.
layout(std140, binding = 3) uniform TemporalBlock
{
  mat4 u_previousView;
  mat4 u_previousProj;
  mat4 u_previousVP;
  mat4 u_prevInvViewProjMatrix;
  vec4 u_prevPosTime;
};

layout(std140, binding = 0) readonly buffer SpotLights
{
  SpotLight sLights[MAX_NUM_LIGHTS];
  uint lightingSettings; // Maximum number of spot lights.
};

layout(std430, binding = 1) restrict readonly buffer LightIndices
{
  int indices[];
};

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  vec4 temporal = fract((u_prevPosTime.wwww * vec4(4.0, 5.0, 6.0, 7.0)) * 0.61803399);
  vec2 uv = (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy);
  return fract(texture(noise, uv) + temporal);
}

float getMiePhase(float cosTheta, float g)
{
  const float scale = 3.0 / (8.0 * PI);

  float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
  float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

// Compute the light attenuation factor. 
// TODO: Shrink the max factor based on froxel world-space size?
float computeAttenuation(vec3 posToLight, float invLightRadius)
{
  float distSquared = dot(posToLight, posToLight);
  float factor = distSquared * invLightRadius * invLightRadius;
  float smoothFactor = max(1.0 - factor * factor, 0.0);
  return (smoothFactor * smoothFactor) / max(distSquared, 1E-1);
}

// Compute the spot light attenuation factor.
float computeAngularAttenuation(vec3 lightDir, vec3 posToLight, 
                                float cosInner, float cosOuter)
{
  return clamp((dot(posToLight, lightDir) - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);  
}

void main()
{
  const ivec2 totalTiles = ivec2(gl_NumWorkGroups.xy / 2); // Same as the total number of tiles.
  const ivec2 currentTile = ivec2(gl_WorkGroupID.xy / 2); // The current tile.
  const uint tileOffset = (totalTiles.x * currentTile.y + currentTile.x) * MAX_NUM_LIGHTS; // Offset into the tile buffer.

  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(inScatExt).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec3 dither = (2.0 * sampleDither(invoke.xy).rgb - 1.0.xxx) / vec3(numFroxels).xyz;
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / vec2(numFroxels.xy) + dither.xy;

  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uv - 1.0.xx, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  vec3 direction = worldSpaceMax - u_camPosition;
  vec3 nDirection = normalize(direction);
  float w = (float(invoke.z) + 0.5) / float(numFroxels.z) + dither.z;
  vec3 worldSpacePostion = u_camPosition + direction * w * w;

  vec3 uvw = vec3(uv, w);

  vec4 se = texture(scatExtinction, uvw);
  vec4 ep = texture(emissionPhase, uvw);
  vec3 totalInScatExt = imageLoad(inScatExt, invoke).rgb;

  // Light the froxel. 
  vec3 mieScattering = se.xyz;
  vec3 extinction = se.www;
  vec3 voxelAlbedo = max(mieScattering / extinction, 0.0.xxx);

  vec3 totalRadiance = totalInScatExt.rgb;
  SpotLight light;
  float phaseFunction, attenuation;
  vec3 lightVector;
  for (int i = 0; i < lightingSettings; i++)
  {
    //int lightIndex = indices[tileOffset + i];

    // If the buffer terminates in -1, quit. No more lights to add to this
    // froxel.
    //if (lightIndex < 0)
    //  break;

    light = sLights[i];
    lightVector = worldSpacePostion - light.positionRange.xyz;
    phaseFunction = getMiePhase(dot(nDirection, normalize(lightVector)), ep.w);
    attenuation = computeAttenuation(lightVector, 1.0 / light.positionRange.w);
    attenuation *= computeAngularAttenuation(normalize(light.direction.xyz), normalize(lightVector), light.cutOffs.x, light.cutOffs.y);

    totalRadiance += voxelAlbedo * phaseFunction * attenuation * light.colourIntensity.rgb * light.colourIntensity.a;
  }

  imageStore(inScatExt, invoke, vec4(totalRadiance, se.w));
}
