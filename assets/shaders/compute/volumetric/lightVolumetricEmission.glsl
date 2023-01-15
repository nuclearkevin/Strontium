#type compute
#version 460 core

#define MAX_NUM_ATMOSPHERES 8
#define MAX_NUM_DYN_SKY_IBL 8
#define PI 3.141592654
#define NUM_CASCADES 4

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Density is not pre-multiplied.
struct DepthFogParams
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  vec4 minMaxDensity; // Minimum density (x) and maximum density (y). z and w are unused.
};

// Density is pre-multiplied.
struct HeightFogParams
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  vec4 falloff; // Falloff (x). y, z and w are unused.
};

// A packed struct for SH coefficients.
struct SHCoefficients
{
  vec4 L00;
  vec4 L11;
  vec4 L10;
  vec4 L1_1;
  vec4 L21;
  vec4 L2_1;
  vec4 L2_2;
  vec4 L20;
  vec4 L22;
  vec4 weightSum; // Sum of weights (x). y, z and w are empty.
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

layout(rgba16f, binding = 0) restrict writeonly uniform image3D inScatExt;

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

layout(std140, binding = 1) uniform VolumetricBlock
{
  DepthFogParams u_depthParams;
  HeightFogParams u_heightParams;
  vec4 u_lightDir; // Light direction (x, y, z). w is unused.
  vec4 u_lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 u_ambientColourIntensity; // Ambient colour (x, y, z) and intensity (w).
  vec4 u_frustumCenter; // Frustum center (x, y, z). w is unused.
  ivec4 u_fogParams; // Number of OBB fog volumes (x), fog parameter bitmask (y). z and w are unused.
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

layout(std140, binding = 4) uniform HillaireParams
{
  AtmosphereParams u_params[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 0) readonly buffer CompactedHarmonics
{
  SHCoefficients compactedSH[];
};

layout(std430, binding = 5) readonly buffer HillaireIndices
{
  int atmosphereIndices[MAX_NUM_ATMOSPHERES];
  int iblIndices[MAX_NUM_DYN_SKY_IBL];
};

const float Y00 = 0.28209479177387814347f; // 1 / (2*sqrt(pi))
const float Y11 = -0.48860251190291992159f; // sqrt(3 /(4pi))
const float Y10 = 0.48860251190291992159f;
const float Y1_1 = -0.48860251190291992159f;
const float Y21 = -1.09254843059207907054f; // 1 / (2*sqrt(pi))
const float Y2_1 = -1.09254843059207907054f;
const float Y2_2 = 1.09254843059207907054f;
const float Y20 = 0.31539156525252000603f; // 1/4 * sqrt(5/pi)
const float Y22 = 0.54627421529603953527f; // 1/4 * sqrt(15/pi)

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  vec4 temporal = fract((u_prevPosTime.wwww * vec4(4.0, 5.0, 6.0, 7.0)) * 0.61803399);
  vec2 uv = (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy);
  return fract(texture(noise, uv) + temporal);
}

float getMiePhase(float cosTheta, float g)
{
  const float scale = 1.0 / (4.0 * PI);

  float num = (1.0 - g * g);
  float denom = pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

float safeACos(float x)
{
  return acos(clamp(x, -1.0, 1.0));
}

vec3 dirToSky(vec3 dir, AtmosphereParams atmo)
{
  float height = length(atmo.viewPos.xyz);
  vec3 up = atmo.viewPos.xyz / height;
  vec3 sunDir = normalize(atmo.sunDirAtmRadius.xyz);

  float horizonAngle = safeACos(sqrt(height * height - atmo.planetAlbedoRadius.w * atmo.planetAlbedoRadius.w) / height);
  float altitudeAngle = horizonAngle - acos(dot(dir, up));

  vec3 right = cross(sunDir, up);
  vec3 forward = cross(up, right);
  vec3 projectedDir = normalize(dir - up * (dot(dir, up)));
  float sinTheta = dot(projectedDir, right);
  float cosTheta = dot(projectedDir, forward);
  float azimuthAngle = atan(sinTheta, cosTheta) + PI;

  vec3 rayDir = normalize(vec3(cos(altitudeAngle) * sin(azimuthAngle),
                               sin(altitudeAngle), -cos(altitudeAngle) * cos(azimuthAngle)));

  return rayDir;
}

vec3 rotateMiePhase(float g, vec3 viewDir)
{
  return vec3(viewDir.y, viewDir.z, viewDir.x) * vec3(g, g, g);
}

// Evaluate the diffuse SH coefficients.
vec3 evaluateSH(SHCoefficients sh, vec3 dir, vec3 miePhaseCoefficients)
{
  vec3 result = 0.0.xxx;
  dir.yzx *= miePhaseCoefficients;

  result += sh.L00.rgb * Y00;

  result += sh.L1_1.rgb * Y1_1 * dir.y;
  result += sh.L10.rgb * Y10 * dir.z;
  result += sh.L11.rgb * Y11 * dir.x;

  result += sh.L2_2.rgb * Y2_2 * (dir.x * dir.y);
  result += sh.L2_1.rgb * Y2_1 * (dir.y * dir.z);
  result += sh.L20.rgb * Y20 * (3.0f * dir.z * dir.z - 1.0f);
  result += sh.L21.rgb * Y21 * (dir.x * dir.z);
  result += sh.L22.rgb * Y22 * (dir.x * dir.x - dir.y * dir.y);

  return max(result, 0.0.xxx);
}

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(inScatExt).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec3 dither = (2.0 * sampleDither(invoke.xy).rgb - 1.0.xxx) / vec3(numFroxels).xyz;
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / vec2(numFroxels.xy) + dither.xy;

  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uv - 1.0.xx, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  vec3 direction = worldSpaceMax - u_camPosition;
  float w = (float(invoke.z) + 0.5) / float(numFroxels.z) + dither.z;
  vec3 worldSpacePostion = u_camPosition + direction * w * w;
  vec3 viewDir = normalize(direction);

  vec3 uvw = vec3(uv, w);

  vec4 se = texture(scatExtinction, uvw);
  vec4 ep = texture(emissionPhase, uvw);
  const float g = ep.w;

  // Fetch the atmosphere.
  const int atmosphereIndex = atmosphereIndices[0];
  const int iblIndex = iblIndices[0];
  const AtmosphereParams atmo = u_params[atmosphereIndex];
  vec3 iblLighting = 1.0.xxx;
  if (iblIndex > -1)
  {
    SHCoefficients sh = compactedSH[iblIndex];
    vec3 dir = normalize(worldSpacePostion - u_frustumCenter.xyz); 
    iblLighting = evaluateSH(sh, dirToSky(dir, atmo), rotateMiePhase(g, dirToSky(viewDir, atmo)));
  }

  // Voxel emission and ambient GI.
  vec3 mieScattering = se.xyz;
  vec3 extinction = se.www;
  vec3 voxelAlbedo = max(mieScattering / extinction, 0.0.xxx);

  vec3 light = ep.xyz;
  light += iblLighting * u_ambientColourIntensity.xyz * u_ambientColourIntensity.w * voxelAlbedo;

  imageStore(inScatExt, invoke, vec4(light, se.w));
}
