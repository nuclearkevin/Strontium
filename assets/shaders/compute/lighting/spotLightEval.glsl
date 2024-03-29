#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of many point
 * lights in tiled deferred shading.
 */

#define MAX_NUM_LIGHTS 512
#define TILE_SIZE 16
#define PI 3.141592654

//#define SHOW_COMPLEXITY

// Launch in 16x16 tiles
layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE) in;

struct SpotLight
{
  vec4 positionRange; // Position (x, y, z), range (w).
  vec4 direction; // Spot light direction (x, y, z). w is empty.
  vec4 cullingSphere; // Sphere center (x, y, z) and radius (w).
  vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
  vec4 cutOffs; // Cos(inner angle) (x), Cos(outer angle) (y). z and w are empty.
};

struct SurfaceProperties
{
  vec3 albedo;
  vec3 metallicF0;
  vec3 dielectricF0;
  float metalness;
  float roughness;
  float ao;

  vec3 position;
  vec3 normal;
  vec3 view;
};

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

layout(std140, binding = 1) readonly buffer SpotLights
{
  SpotLight sLights[MAX_NUM_LIGHTS];
  uint lightingSettings; // Maximum number of spot lights.
};

layout(std430, binding = 2) readonly buffer LightIndices
{
  int indices[];
};

layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

// Uniforms for the geometry buffer.
layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMatProp;

// Decode the g-buffer.
SurfaceProperties decodeGBuffer(vec2 gBufferUVs);

// Evaluate a single spot light.
vec3 evaluateSpotLight(SpotLight light, SurfaceProperties props);

// Debug gradient for light complexity.
vec3 gradient(float value);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  const ivec2 totalTiles = ivec2(gl_NumWorkGroups.xy); // Same as the total number of tiles.
  const ivec2 currentTile = ivec2(gl_WorkGroupID.xy); // The current tile.
  const uint tileOffset = (totalTiles.x * currentTile.y + currentTile.x) * MAX_NUM_LIGHTS; // Offset into the tile buffer.

  vec2 screenSize = vec2(textureSize(gDepth, 0).xy);
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / screenSize;
  SurfaceProperties gBuffer = decodeGBuffer(gBufferUVs);

  // Loop over the lights and compute the radiance contribution.
  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;
  #ifdef SHOW_COMPLEXITY
  uint numLights = 0;
  #endif
  for (int i = 0; i < lightingSettings; i++)
  {
    int lightIndex = indices[tileOffset + i];

    // If the buffer terminates in -1, quit. No more lights to add to this
    // fragment.
    if (lightIndex < 0)
      break;

    #ifdef SHOW_COMPLEXITY
    numLights++;
    #endif

    totalRadiance += evaluateSpotLight(sLights[lightIndex], gBuffer);
  }

  #ifdef SHOW_COMPLEXITY
  totalRadiance += gradient(float(numLights));
  #endif

  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
}

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(vec2 texCoords, sampler2D depthMap, mat4 invMVP)
{
  float depth = texture(depthMap, texCoords).r;
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invMVP * vec4(clipCoords, 1.0);
  return temp.xyz / temp.w;
}
// Fast octahedron normal vector decoding.
// https://jcgt.org/published/0003/02/01/
vec2 signNotZero(vec2 v)
{
  return vec2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
}
vec3 decodeNormal(vec2 texCoords, sampler2D encodedNormals)
{
  vec2 e = texture(encodedNormals, texCoords).xy;
  vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
  if (v.z < 0)
    v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
  return normalize(v);
}

SurfaceProperties decodeGBuffer(vec2 gBufferUVs)
{
  SurfaceProperties decoded;
  decoded.position = decodePosition(gBufferUVs, gDepth, u_invViewProjMatrix);
  decoded.view = normalize(u_camPosition - decoded.position);
  decoded.normal = decodeNormal(gBufferUVs, gNormal);

  vec3 mra = texture(gMatProp, gBufferUVs).rgb;
  decoded.metalness = mra.r;
  decoded.roughness = mra.g;
  decoded.ao = mra.b;

  // Remap material properties.
  vec4 albedoReflectance = texture(gAlbedo, gBufferUVs).rgba;
  decoded.albedo = albedoReflectance.rgb;
  decoded.dielectricF0 = 0.16 * albedoReflectance.aaa * albedoReflectance.aaa;
  decoded.metallicF0 = decoded.albedo * decoded.metalness;

  return decoded;
}

//------------------------------------------------------------------------------
// Filament PBR.
//------------------------------------------------------------------------------
// Compute the light attenuation factor.
float computeAttenuation(vec3 posToLight, float invLightRadius)
{
  float distSquared = dot(posToLight, posToLight);
  float factor = distSquared * invLightRadius * invLightRadius;
  float smoothFactor = max(1.0 - factor * factor, 0.0);
  return (smoothFactor * smoothFactor) / max(distSquared, 1e-4);
}

// Compute the spot light attenuation factor.
float computeAngularAttenuation(vec3 lightDir, vec3 posToLight, 
                                float cosInner, float cosOuter)
{
  return clamp((dot(posToLight, -lightDir) - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);  
}

// Normal distribution function.
float nGGX(float nDotH, float actualRoughness)
{
  float a = nDotH * actualRoughness;
  float k = actualRoughness / (1.0 - nDotH * nDotH + a * a);
  return k * k * (1.0 / PI);
}

// Fast visibility term. Incorrect as it approximates the two square roots.
float vGGXFast(float nDotV, float nDotL, float actualRoughness)
{
  float a = actualRoughness;
  float vVGGX = nDotL * (nDotV * (1.0 - a) + a);
  float lVGGX = nDotV * (nDotL * (1.0 - a) + a);
  return 0.5 / max(vVGGX + lVGGX, 1e-5);
}

// Schlick approximation for the Fresnel factor.
vec3 sFresnel(float vDotH, vec3 f0, vec3 f90)
{
  float p = 1.0 - vDotH;
  return f0 + (f90 - f0) * p * p * p * p * p;
}

// Cook-Torrance specular for the specular component of the BRDF.
vec3 fsCookTorrance(float nDotH, float lDotH, float nDotV, float nDotL,
                    float vDotH, float actualRoughness, vec3 f0, vec3 f90)
{
  float D = nGGX(nDotH, actualRoughness);
  vec3 F = sFresnel(vDotH, f0, f90);
  float V = vGGXFast(nDotV, nDotL, actualRoughness);
  return D * F * V;
}

// Lambertian diffuse for the diffuse component of the BRDF. Corrected to guarantee
// energy is conserved.
vec3 fdLambertCorrected(vec3 f0, vec3 f90, float vDotH, float lDotH,
                        vec3 diffuseAlbedo)
{
  // Making the assumption that the external medium is air (IOR of 1).
  vec3 iorExtern = vec3(1.0);
  // Calculating the IOR of the medium using f0.
  vec3 iorIntern = (vec3(1.0) - sqrt(f0)) / (vec3(1.0) + sqrt(f0));
  // Ratio of the IORs.
  vec3 iorRatio = iorExtern / iorIntern;

  // Compute the incoming and outgoing Fresnel factors.
  vec3 fIncoming = sFresnel(lDotH, f0, f90);
  vec3 fOutgoing = sFresnel(vDotH, f0, f90);

  // Compute the fraction of light which doesn't get reflected back into the
  // medium for TIR.
  vec3 rExtern = PI * (20.0 * f0 + 1.0) / 21.0;
  // Use rExtern to compute the fraction of light which gets reflected back into
  // the medium for TIR.
  vec3 rIntern = vec3(1.0) - (iorRatio * iorRatio * (vec3(1.0) - rExtern));

  // The TIR contribution.
  vec3 tirDiffuse = vec3(1.0) - (rIntern * diffuseAlbedo);

  // The final diffuse BRDF.
  return (iorRatio * iorRatio) * diffuseAlbedo * (vec3(1.0) - fIncoming) * (vec3(1.0) - fOutgoing) / (PI * tirDiffuse);
}

// The final combined BRDF. Compensates for energy gain in the diffuse BRDF and
// energy loss in the specular BRDF.
vec3 filamentBRDF(vec3 l, vec3 v, vec3 n, float roughness, float metallic,
                  vec3 dielectricF0, vec3 metallicF0, vec3 f90,
                  vec3 diffuseAlbedo)
{
  vec3 h = normalize(v + l);

  float nDotV = max(abs(dot(n, v)), 1e-5);
  float nDotL = clamp(dot(n, l), 1e-5, 1.0);
  float nDotH = clamp(dot(n, h), 1e-5, 1.0);
  float lDotH = clamp(dot(l, h), 1e-5, 1.0);
  float vDotH = clamp(dot(v, h), 1e-5, 1.0);

  float clampedRoughness = max(roughness, 0.045);
  float actualRoughness = clampedRoughness * clampedRoughness;

  vec3 fs = fsCookTorrance(nDotH, lDotH, nDotV, nDotL, vDotH, actualRoughness, dielectricF0, f90);
  vec3 fd = fdLambertCorrected(dielectricF0, f90, vDotH, lDotH, diffuseAlbedo);
  vec3 dielectricBRDF = fs + fd;

  vec3 metallicBRDF = fsCookTorrance(nDotH, lDotH, nDotV, nDotL, vDotH, actualRoughness, metallicF0, f90);

  return mix(dielectricBRDF, metallicBRDF, metallic);
}

//------------------------------------------------------------------------------
// Compute an individual spot light contribution.
//------------------------------------------------------------------------------
vec3 evaluateSpotLight(SpotLight light, SurfaceProperties props)
{
  vec3 posToLight = light.positionRange.xyz - props.position;
  vec3 lightDir = normalize(posToLight);
  vec3 halfWay = normalize(props.view + lightDir);
  float nDotL = clamp(dot(props.normal, lightDir), 0.0, 1.0);

  float lightRange = light.positionRange.w;
  float attenuation = computeAttenuation(posToLight, 1.0 / light.positionRange.w);
  attenuation *= computeAngularAttenuation(normalize(light.direction.xyz), normalize(posToLight), light.cutOffs.x, light.cutOffs.y);
  vec3 lightColour = light.colourIntensity.xyz;
  float lightIntensity = light.colourIntensity.w;

  // Compute the radiance contribution.
  vec3 radiance = filamentBRDF(lightDir, props.view, props.normal, props.roughness,
                               props.metalness, props.dielectricF0, props.metallicF0,
                               vec3(1.0), props.albedo);

  return radiance * lightIntensity * lightColour * nDotL * attenuation;
}

vec3 gradient(float value)
{
  vec3 zero = vec3(0.0, 0.0, 0.0);
  vec3 white = vec3(0.0, 0.1, 0.9);
  vec3 red = vec3(0.2, 0.9, 0.4);
  vec3 blue = vec3(0.8, 0.8, 0.3);
  vec3 green = vec3(0.9, 0.2, 0.3);

  float step0 = 0.0;
  float step1 = 2.0;
  float step2 = 4.0;
  float step3 = 8.0;
  float step4 = 16.0;

  vec3 color = mix(zero, white, smoothstep(step0, step1, value));
  color = mix(color, white, smoothstep(step1, step2, value));
  color = mix(color, red, smoothstep(step1, step2, value));
  color = mix(color, blue, smoothstep(step2, step3, value));
  color = mix(color, green, smoothstep(step3, step4, value));

  return color;
}