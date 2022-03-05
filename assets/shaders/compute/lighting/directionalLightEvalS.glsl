#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of up to 8
 * directional lights in deferred shading. Evaluates a single shadowed
 * directional light (for now).
 */

#define PI 3.141592654

#define MAX_NUM_LIGHTS 8
#define NUM_CASCADES 4

#define PCF_SAMPLES 64
#define PCSS_SAMPLES 16

layout(local_size_x = 8, local_size_y = 8) in;

struct DirectionalLight
{
  vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
  vec4 directionSize; // Light direction (x, y, z) and light size (w).
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
  vec4 u_nearFar; // Near plane (x), far plane (y), gamma (z). w is unused.
};

// Directional light uniforms.
layout(std140, binding = 1) uniform DirectionalBlock
{
  DirectionalLight dLights[MAX_NUM_LIGHTS];
  // Number of directional lights (x) with a maximum of 8, lighting settings
  // bitmask (y), the directional light to be shadowed (z), and the shadow quality (w).
  ivec4 u_lightingSettings;
};

layout(std140, binding = 2) uniform CascadedShadowBlock
{
  mat4 u_lightVP[NUM_CASCADES];
  vec4 u_cascadeData[NUM_CASCADES]; // Cascade split distance (x). y, z and w are unused.
  vec4 u_shadowParams; // The constant bias (x), normal bias (y), and the minimum PCF radius (z). w is unused.
};

layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

// Uniforms for the geometry buffer.
layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMatProp;
// TODO: Texture arrays.
layout(binding = 4) uniform sampler2D cascadeMaps[NUM_CASCADES];

// Decode the g-buffer.
SurfaceProperties decodeGBuffer(vec2 gBufferUVs);

// Evaluate a single directional light.
vec3 evaluateDirectionalLight(DirectionalLight light, SurfaceProperties props);

// Shadow calculations.
float calcShadow(uint cascadeIndex, vec3 position, vec3 normal, DirectionalLight light,
                 int quality);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  if (texture(gDepth, gBufferUVs).r == 1.0)
    return;

  SurfaceProperties gBuffer = decodeGBuffer(gBufferUVs);
  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;

  // Cascaded shadow mapping.
  // Make the assumption the light to shadow is at index 0.
  const DirectionalLight shadowedLight = dLights[0];

  vec4 clipSpacePos = u_viewMatrix * vec4(gBuffer.position, 1.0);
  float shadowFactor = 1.0;
  for (uint i = 0; i < NUM_CASCADES; i++)
  {
    if (-clipSpacePos.z <= (u_cascadeData[i].x))
    {
      // Interpolate between the current and the next cascade to smooth the
      // transition between shadows.
      shadowFactor = calcShadow(i, gBuffer.position, gBuffer.normal, shadowedLight,
                                u_lightingSettings.w);

      float currentSplit = i == 0 ? 0.0 : u_cascadeData[i - 1].x;
      float nextSplit = u_cascadeData[i].x;
      float splitDelta = nextSplit - currentSplit;
      float fadeFactor = (nextSplit + clipSpacePos.z) / splitDelta;

      if (i < (NUM_CASCADES - 1))
      {
        float nextShadowFactor = calcShadow(i + 1, gBuffer.position, gBuffer.normal, shadowedLight,
                                            u_lightingSettings.w);
        float lerpFactor = smoothstep(0.0, 1.0, fadeFactor);
        shadowFactor = mix(nextShadowFactor, shadowFactor, lerpFactor);
      }

      break;
    }
  }

  totalRadiance += shadowFactor * evaluateDirectionalLight(shadowedLight, gBuffer);

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
// Compute an individual directional light contribution.
//------------------------------------------------------------------------------
vec3 evaluateDirectionalLight(DirectionalLight light, SurfaceProperties props)
{
  vec3 lightDir = normalize(light.directionSize.xyz);
  vec3 halfWay = normalize(props.view + lightDir);
  float nDotL = clamp(dot(props.normal, lightDir), 0.0, 1.0);

  vec3 lightColour = light.colourIntensity.xyz;
  float lightIntensity = light.colourIntensity.w;

  // Compute the radiance contribution.
  vec3 radiance = filamentBRDF(lightDir, props.view, props.normal, props.roughness,
                               props.metalness, props.dielectricF0, props.metallicF0,
                               vec3(1.0), props.albedo);

  return radiance * lightIntensity * lightColour * nDotL;
}

//------------------------------------------------------------------------------
// Shadow helper functions.
//------------------------------------------------------------------------------
// Interleaved gradient noise from:
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float interleavedGradientNoise()
{
  const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
  //return fract(magic.z * fract(dot(gBufferUVs.xy, magic.xy)));
  return fract(magic.z * fract(dot(vec2(gl_GlobalInvocationID.xy), magic.xy)));
}

// Create a rotation matrix with the noise function above.
mat2 randomRotation()
{
  float theta = 2.0 * PI * interleavedGradientNoise();
  float sinTheta = sin(theta);
  float cosTheta = cos(theta);
  return mat2(cosTheta, sinTheta, -sinTheta, cosTheta);
}

const vec2 poissonDisk[64] = vec2[64]
(
  vec2(-0.613392, 0.617481),
  vec2(0.170019, -0.040254),
  vec2(-0.299417, 0.791925),
  vec2(0.645680, 0.493210),
  vec2(-0.651784, 0.717887),
  vec2(0.421003, 0.027070),
  vec2(-0.817194, -0.271096),
  vec2(-0.705374, -0.668203),
  vec2(0.977050, -0.108615),
  vec2(0.063326, 0.142369),
  vec2(0.203528, 0.214331),
  vec2(-0.667531, 0.326090),
  vec2(-0.098422, -0.295755),
  vec2(-0.885922, 0.215369),
  vec2(0.566637, 0.605213),
  vec2(0.039766, -0.396100),
  vec2(0.751946, 0.453352),
  vec2(0.078707, -0.715323),
  vec2(-0.075838, -0.529344),
  vec2(0.724479, -0.580798),
  vec2(0.222999, -0.215125),
  vec2(-0.467574, -0.405438),
  vec2(-0.248268, -0.814753),
  vec2(0.354411, -0.887570),
  vec2(0.175817, 0.382366),
  vec2(0.487472, -0.063082),
  vec2(-0.084078, 0.898312),
  vec2(0.488876, -0.783441),
  vec2(0.470016, 0.217933),
  vec2(-0.696890, -0.549791),
  vec2(-0.149693, 0.605762),
  vec2(0.034211, 0.979980),
  vec2(0.503098, -0.308878),
  vec2(-0.016205, -0.872921),
  vec2(0.385784, -0.393902),
  vec2(-0.146886, -0.859249),
  vec2(0.643361, 0.164098),
  vec2(0.634388, -0.049471),
  vec2(-0.688894, 0.007843),
  vec2(0.464034, -0.188818),
  vec2(-0.440840, 0.137486),
  vec2(0.364483, 0.511704),
  vec2(0.034028, 0.325968),
  vec2(0.099094, -0.308023),
  vec2(0.693960, -0.366253),
  vec2(0.678884, -0.204688),
  vec2(0.001801, 0.780328),
  vec2(0.145177, -0.898984),
  vec2(0.062655, -0.611866),
  vec2(0.315226, -0.604297),
  vec2(-0.780145, 0.486251),
  vec2(-0.371868, 0.882138),
  vec2(0.200476, 0.494430),
  vec2(-0.494552, -0.711051),
  vec2(0.612476, 0.705252),
  vec2(-0.578845, -0.768792),
  vec2(-0.772454, -0.090976),
  vec2(0.504440, 0.372295),
  vec2(0.155736, 0.065157),
  vec2(0.391522, 0.849605),
  vec2(-0.620106, -0.328104),
  vec2(0.789239, -0.419965),
  vec2(-0.545396, 0.538133),
  vec2(-0.178564, -0.596057)
);

vec2 samplePoissonDisk(uint index)
{
  return poissonDisk[index % 64];
}

//------------------------------------------------------------------------------
// Shadow calculations for exponentially warped variance shadow maps.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Shadow calculations for PCF shadow maps.
//------------------------------------------------------------------------------
float offsetLookup(sampler2D map, vec2 loc, vec2 offset, float depth)
{
  vec2 texel = 1.0 / textureSize(map, 0).xy;
  return float(texture(map, loc.xy + offset * texel).r >= depth);
}

//------------------------------------------------------------------------------
// Shadow calculations for PCSS shadow maps.
//------------------------------------------------------------------------------
float calculateSearchWidth(float receiverDepth, float lightSize, vec2 texelSize)
{
  return lightSize * receiverDepth;
}

float calcBlockerDistance(sampler2D map, vec3 projCoords, float bias, float lightSize)
{
  vec2 texel = 1.0 / textureSize(map, 0).xy;
  float blockerDistances = 0.0;
  float numBlockerDistances = 0.0;
  float receiverDepth = projCoords.z;

  float searchWidth = calculateSearchWidth(receiverDepth, lightSize, texel);
  mat2 rotation = randomRotation();
  for (uint i = 0; i < PCSS_SAMPLES; i++)
  {
    vec2 disk = searchWidth * samplePoissonDisk(i);
    disk = rotation * disk;

    float blockerDepth = texture(map, projCoords.xy + disk * texel).r;
    if (blockerDepth < projCoords.z - bias)
    {
      numBlockerDistances += 1.0;
      blockerDistances += blockerDepth;
    }
  }

  if (numBlockerDistances > 0.0)
    return blockerDistances / numBlockerDistances;
  else
    return -1.0;
}

float calcPCFKernelSize(sampler2D map, vec3 projCoords, float bias, float lightSize)
{
  float receiverDepth = projCoords.z;
  float blockerDepth = calcBlockerDistance(map, projCoords, bias, lightSize);

  float kernelSize = 1.0;
  if (blockerDepth > 0.0)
  {
    float penumbraWidth = lightSize * (receiverDepth - blockerDepth) / blockerDepth;
    kernelSize = penumbraWidth / (receiverDepth);
  }

  return kernelSize;
}

//------------------------------------------------------------------------------
// Compute the actual occlusion factor.
//------------------------------------------------------------------------------
float calcShadow(uint cascadeIndex, vec3 position, vec3 normal, DirectionalLight light,
                 int quality)
{
  float depthRange = 0.0;
  if (cascadeIndex == 0)
  {
    depthRange = u_cascadeData[cascadeIndex].x;
  }
  else
  {
    depthRange = u_cascadeData[cascadeIndex].x - u_cascadeData[cascadeIndex - 1].x;
  }

  float bias = u_shadowParams.x / 1000.0;

  float normalOffsetScale = clamp(1.0 - dot(normal, light.directionSize.xyz), 0.0, 1.0);
  normalOffsetScale /= textureSize(cascadeMaps[cascadeIndex], 0).x;
  normalOffsetScale *= depthRange * u_shadowParams.y;

  vec4 shadowOffset = vec4(position + normal * normalOffsetScale, 1.0);
  shadowOffset = u_lightVP[cascadeIndex] * shadowOffset;

  vec4 lightClipPos = u_lightVP[cascadeIndex] * vec4(position, 1.0);
  lightClipPos.xy = shadowOffset.xy;
  vec3 projCoords = lightClipPos.xyz / lightClipPos.w;
  projCoords = 0.5 * projCoords + 0.5;

  // PCSS (contact hardening shadows), ultra quality.
  float shadowFactor = 1.0;
  if (quality == 2)
  {
    shadowFactor = 0.0;
    float radius = calcPCFKernelSize(cascadeMaps[cascadeIndex], projCoords, bias, light.directionSize.w);
    radius += u_shadowParams.z;
    mat2 rotation = randomRotation();
    for (uint i = 0; i < PCF_SAMPLES; i++)
    {
      vec2 disk = radius * samplePoissonDisk(i);
      disk = rotation * disk;

      shadowFactor += offsetLookup(cascadeMaps[cascadeIndex], projCoords.xy,
                                   disk, projCoords.z - bias);
    }

    shadowFactor /= float(PCF_SAMPLES);
  }
  // PCF shadows, medium quality.
  else if (quality == 1)
  {
    shadowFactor = 0.0;
    float radius = u_shadowParams.z;
    mat2 rotation = randomRotation();
    for (uint i = 0; i < PCF_SAMPLES; i++)
    {
      vec2 disk = radius * samplePoissonDisk(i);
      disk = rotation * disk;
      shadowFactor += offsetLookup(cascadeMaps[cascadeIndex], projCoords.xy,
                                   disk, projCoords.z - bias);
    }

    shadowFactor /= float(PCF_SAMPLES);
  }
  // Hard shadows, low quality.
  else if (quality == 0)
  {
    shadowFactor = offsetLookup(cascadeMaps[cascadeIndex], projCoords.xy, 0.0.xx,
                                projCoords.z - bias);
  }

  return shadowFactor;
}
