#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of up to 8
 * directional lights in deferred shading. This shader also accounts for
 * atmospheric effects.
 * TODO: 3D transmittance LUT.
 */

#define PI 3.141592654

#define MAX_NUM_ATMOSPHERES 8
#define MAX_NUM_LIGHTS 8

layout(local_size_x = 8, local_size_y = 8) in;

struct DirectionalLight
{
  vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
  vec4 direction; // Light direction (x, y, z) and light size (w).
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

// Atmospheric scattering coefficients are expressed per unit km.
struct ScatteringParams
{
  vec4 rayleighScat; //  Rayleigh scattering base (x, y, z) and height falloff (w).
  vec4 rayleighAbs; //  Rayleigh absorption base (x, y, z) and height falloff (w).
  vec4 mieScat; //  Mie scattering base (x, y, z) and height falloff (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and height falloff (w).
  vec4 ozoneAbs; //  Ozone absorption base (x, y, z) and scale (w).
};

// Radii are expressed in megameters (MM).
struct AtmosphereParams
{
  ScatteringParams sParams;
  vec4 planetAlbedoRadius; // Planet albedo (x, y, z) and radius.
  vec4 sunDirAtmRadius; // Sun direction (x, y, z) and atmosphere radius (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 viewPos; // View position (x, y, z). w is unused.
};

// The lighting buffer.
layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

// Uniforms for the geometry buffer.
layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMatProp;
layout(binding = 8) uniform sampler2DArray transLUTs;

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
  ivec4 u_lightingSettings; // Number of directional lights (x) with a maximum of 8. y, z and w are unused.
};

layout(std140, binding = 3) uniform HillaireParams
{
  AtmosphereParams u_params[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 4) readonly buffer HillaireIndices
{
  int u_atmosphereIndices[MAX_NUM_ATMOSPHERES];
};

// Decode the g-buffer.
SurfaceProperties decodeGBuffer(vec2 gBufferUVs);

// Decode a Hillaire2020 LUT.
vec3 getValFromLUT(float atmIndex, sampler2DArray tex, vec3 pos, vec3 sunDir,
                   float groundRadiusMM, float atmosphereRadiusMM);

// Evaluate a single directional light.
vec3 evaluateDirectionalLight(DirectionalLight light, SurfaceProperties props);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  if (texture(gDepth, gBufferUVs).r >= (1.0 - 1e-6))
    return;

  SurfaceProperties gBuffer = decodeGBuffer(gBufferUVs);

  // Loop over the lights and compute the radiance contribution for everything
  // that isn't the shadowed light.
  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;
  for (int i = 0; i < u_lightingSettings.x; i++)
  {
    // Get the light.
    const DirectionalLight light = dLights[i];

    // Fetch the atmosphere.
    const int atmosphereIndex = u_atmosphereIndices[i];
    const AtmosphereParams params = u_params[atmosphereIndex];

    // Fetch the transmittance.
    vec3 sunTransmittance = getValFromLUT(float(atmosphereIndex), transLUTs,
                                          params.viewPos.xyz,
                                          normalize(light.direction.xyz),
                                          params.planetAlbedoRadius.w,
                                          params.sunDirAtmRadius.w);

    // Multiply by the transmittance from the sun -> object.
    totalRadiance += sunTransmittance * evaluateDirectionalLight(light, gBuffer);
  }

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

// Decode a Hillaire2020 LUT.
vec3 getValFromLUT(float atmIndex, sampler2DArray tex, vec3 pos, vec3 sunDir,
                   float groundRadiusMM, float atmosphereRadiusMM)
{
  float height = length(pos);
  vec3 up = pos / height;

  float sunCosZenithAngle = dot(sunDir, up);

  float u = clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0);
  float v = max(0.0, min(1.0, (height - groundRadiusMM) / (atmosphereRadiusMM - groundRadiusMM)));

  return texture(tex, vec3(u, v, atmIndex)).rgb;
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
// Compute an individual point light contribution.
//------------------------------------------------------------------------------
vec3 evaluateDirectionalLight(DirectionalLight light, SurfaceProperties props)
{
  vec3 lightDir = normalize(light.direction.xyz);
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
