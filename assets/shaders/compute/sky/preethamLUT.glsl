#type compute
#version 460 core
/*
 * A compute shader to generate a look up texture for the Preetham skybox.
 * Adapted from https://www.shadertoy.com/view/llSSDR.
*/

#define TWO_PI 6.283185308
#define PI 3.141592654
#define PI_OVER_TWO 1.570796327

layout(local_size_x = 8, local_size_y = 8) in;

// The next mip in the downsampling mip chain.
layout(rgba16f, binding = 0) restrict writeonly uniform image2D writeImage;

// The parameters for the LUT.
layout(std140, binding = 1) buffer PreethamParams
{
  vec4 u_sunDirTurbidity; // Sun direction (x, y, z) and the turbidity (w).
  vec4 u_sunIntensitySize; // Sun intensity (x) and size (y). z and w are unused.
};

// Helper functions.
float saturatedDot(vec3 a, vec3 b);
vec3 YxyToXYZ(vec3 Yxy);
vec3 XYZToRGB(vec3 XYZ);
vec3 YxyToRGB(vec3 Yxy);

// Functions to compute the sky.
void calculatePerezDistribution(float t, out vec3 A, out vec3 B, out vec3 C, out vec3 D, out vec3 E);
vec3 calculateZenithLuminanceYxy(float t, float thetaS);
vec3 calculatePerezLuminanceYxy(float theta, float gamma, vec3 A, vec3 B, vec3 C, vec3 D, vec3 E);
vec3 calculateSkyLuminanceRGB(vec3 s, vec3 e, float t);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 writeImageSize = vec2(imageSize(writeImage).xy);

  float u = float(invoke.x) / writeImageSize.x;
  float v = float(invoke.y) / writeImageSize.y;

  // U component of the LUT is the azimuthal angle divided by 2 * Pi. Ranges
  // between 0 and 2 * Pi.
  float viewAzimuth = (u - 0.5) * TWO_PI;

  // V component of the LUT is the view inclination from the horizon,
  // divided by Pi. Ranges between -Pi / 2 and Pi / 2.
  float viewInclination = (2.0 * v - 1.0) * PI_OVER_TWO;

  // Reconstruct the view vector.
  float cosInclination = cos(viewInclination);
  vec3 viewDir = normalize(vec3(cosInclination * sin(viewAzimuth), sin(viewInclination), -cosInclination * cos(viewAzimuth)));

  // Sun direction and the sky turbidity.
  vec3 sunDir = normalize(u_sunDirTurbidity.xyz);
  float turbidity = u_sunDirTurbidity.w;

  // Compute the luminance.
  vec3 skyLuminance = 0.05 * calculateSkyLuminanceRGB(sunDir, viewDir, turbidity);

  imageStore(writeImage, invoke, vec4(skyLuminance, 1.0));
}

float saturatedDot(vec3 a, vec3 b)
{
	return max(dot(a, b), 0.0);
}

vec3 YxyToXYZ(vec3 Yxy)
{
  float Y = Yxy.r;
  float x = Yxy.g;
  float y = Yxy.b;

  float X = x * (Y / y);
  float Z = (1.0 - x - y) * (Y / y);

  return vec3(X,Y,Z);
}

vec3 XYZToRGB(vec3 XYZ)
{
  // CIE/E
  mat3 M = mat3
  (
    2.3706743, -0.9000405, -0.4706338,
    -0.5138850,  1.4253036,  0.0885814,
    0.0052982, -0.0146949,  1.0093968
  );

  return XYZ * M;
}

vec3 YxyToRGB(vec3 Yxy)
{
  vec3 XYZ = YxyToXYZ(Yxy);
  vec3 RGB = XYZToRGB(XYZ);
  return RGB;
}

void calculatePerezDistribution(float t, out vec3 A, out vec3 B, out vec3 C,
                                out vec3 D, out vec3 E)
{
  A = vec3(0.1787 * t - 1.4630, -0.0193 * t - 0.2592, -0.0167 * t - 0.2608);
  B = vec3(-0.3554 * t + 0.4275, -0.0665 * t + 0.0008, -0.0950 * t + 0.0092);
  C = vec3(-0.0227 * t + 5.3251, -0.0004 * t + 0.2125, -0.0079 * t + 0.2102);
  D = vec3(0.1206 * t - 2.5771, -0.0641 * t - 0.8989, -0.0441 * t - 1.6537);
  E = vec3(-0.0670 * t + 0.3703, -0.0033 * t + 0.0452, -0.0109 * t + 0.0529);
}

vec3 calculateZenithLuminanceYxy(float t, float thetaS)
{
  float chi = (4.0 / 9.0 - t / 120.0) * (PI - 2.0 * thetaS);
  float Yz = (4.0453 * t - 4.9710) * tan(chi) - 0.2155 * t + 2.4192;

  float theta2 = thetaS * thetaS;
  float theta3 = theta2 * thetaS;
  float T = t;
  float T2 = t * t;

  float xz =
    (0.00165 * theta3 - 0.00375 * theta2 + 0.00209 * thetaS + 0.0) * T2 +
    (-0.02903 * theta3 + 0.06377 * theta2 - 0.03202 * thetaS + 0.00394) * T +
    (0.11693 * theta3 - 0.21196 * theta2 + 0.06052 * thetaS + 0.25886);

  float yz =
    (0.00275 * theta3 - 0.00610 * theta2 + 0.00317 * thetaS + 0.0) * T2 +
    (-0.04214 * theta3 + 0.08970 * theta2 - 0.04153 * thetaS + 0.00516) * T +
    (0.15346 * theta3 - 0.26756 * theta2 + 0.06670 * thetaS + 0.26688);

  return vec3(Yz, xz, yz);
}

vec3 calculatePerezLuminanceYxy(float theta, float gamma, vec3 A, vec3 B, vec3 C, vec3 D, vec3 E)
{
  return (1.0 + A * exp(B / cos(theta))) * (1.0 + C * exp(D * gamma) + E * cos(gamma) * cos(gamma));
}

// s = sun direction, e = view direction.
vec3 calculateSkyLuminanceRGB(vec3 s, vec3 e, float t)
{
  vec3 A, B, C, D, E;
  calculatePerezDistribution(t, A, B, C, D, E);

  float thetaS = acos(saturatedDot(s, vec3(0.0, 1.0, 0.0)));
  float thetaE = acos(saturatedDot(e, vec3(0.0, 1.0, 0.0)));
  float gammaE = acos(saturatedDot(s, e));

  vec3 Yz = calculateZenithLuminanceYxy(t, thetaS);

  vec3 fThetaGamma = calculatePerezLuminanceYxy(thetaE, gammaE, A, B, C, D, E);
  vec3 fZeroThetaS = calculatePerezLuminanceYxy(0.0, thetaS, A, B, C, D, E);

  vec3 Yp = Yz * (fThetaGamma / fZeroThetaS);

  return YxyToRGB(Yp);
}
