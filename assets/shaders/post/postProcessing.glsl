#type common
#version 460 core
/*
 * The post-processing megashader program.
*/

#type vertex
void main()
{
  vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;

  gl_Position = vec4(position, 0.0, 1.0);
}

#type fragment
#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL 0.125
#define FXAA_REDUCE_MIN 0.0078125 // 1.0 / 180.0

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

// The post processing properties.
layout(std140, binding = 1) uniform PostProcessBlock
{
  vec4 u_bloom;  // Bloom intensity (x) and radius (y), z and w are unused.
  ivec2 u_postSettings; // Using FXAA (bit 1), using bloom (bit 2) (x). Tone mapping operator (y). z and w are unused.
};

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gEntityIDMask;
layout(binding = 2) uniform sampler2D screenColour;
layout(binding = 3) uniform sampler2D bloomColour;

// Output colour variable.
layout(location = 0) out vec4 fragColour;
layout(location = 1) out float fragID;

// Bloom.
vec3 upsampleBoxTent(sampler2D bloomTexture, vec2 uv, float radius);
// FXAA.
vec3 applyFXAA(vec2 uv, bool useBloom, bool useGrid, bool useOutline);
// Tone mapping.
vec3 toneMap(vec3 colour, uint operator);
// Gamma correct.
vec3 applyGamma(vec3 colour, float gamma);

// Stuff for the editor.
// Grid.
vec3 applyGrid(vec3 colour, sampler2D gDepth, vec2 uvs, mat4 invVP, mat4 vP);
// The outline.
vec3 applyOutline(vec3 colour, sampler2D idMask, vec2 uvs);

// Dithering to deband colours.
vec3 screenSpaceDither(vec2 uvs);

void main()
{
  vec2 screenSize = vec2(textureSize(screenColour, 0).xy);
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;

  bool useFXAA = (u_postSettings.x & (1 << 0)) != 0;
  bool useBloom = (u_postSettings.x & (1 << 1)) != 0;
  bool useGrid = (u_postSettings.x & (1 << 2)) != 0;
  bool useOutline = (u_postSettings.x & (1 << 3)) != 0;

  vec3 colour = 0.0.xxx;
  if (useFXAA)
    colour = applyFXAA(fTexCoords, useBloom, useGrid, useOutline);
  else
  {
    colour = texture(screenColour, fTexCoords).rgb;
    if (useBloom)
      colour += u_bloom.x * upsampleBoxTent(bloomColour, fTexCoords, u_bloom.y);
    if (useGrid)
      colour = applyGrid(colour, gDepth, fTexCoords, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    if (useOutline)
      colour = applyOutline(colour, gEntityIDMask, fTexCoords);

    colour = toneMap(colour, uint(u_postSettings.y));
    colour = applyGamma(colour, u_nearFarGamma.z);
  }

  // Dither to improve banding.
  colour = colour + screenSpaceDither(gl_FragCoord.xy);

  fragColour = vec4(colour, 1.0);
  fragID = texture(gEntityIDMask, fTexCoords).g;
}

// Helper functions.
float rgbToLuma(vec3 rgbColour)
{
  return dot(rgbColour, vec3(0.299, 0.587, 0.114));
}

/*
  A  B  C
  D  E  F
  G  H  I
*/
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
vec3 upsampleBoxTent(sampler2D bloomTexture, vec2 uv, float radius)
{
  vec2 texelSize = 1.0.xx / vec2(textureSize(bloomTexture, 0).xy);
  vec4 d = texelSize.xyxy * vec4(1.0, 1.0, -1.0, 0.0) * 1.0;

  vec3 A = texture(bloomTexture, uv - d.xy).rgb;
  vec3 B = texture(bloomTexture, uv - d.wy).rgb * 2.0;
  vec3 C = texture(bloomTexture, uv - d.zy).rgb;
  vec3 D = texture(bloomTexture, uv + d.zw).rgb * 2.0;
  vec3 E = texture(bloomTexture, uv).rgb * 4.0;
  vec3 F = texture(bloomTexture, uv + d.xw).rgb * 2.0;
  vec3 G = texture(bloomTexture, uv + d.zy).rgb;
  vec3 H = texture(bloomTexture, uv + d.wy).rgb * 2.0;
  vec3 I = texture(bloomTexture, uv + d.xy).rgb;

  return (A + B + C + D + E + F + G + H + I) * 0.0625; // * 1/16
}

// FXAA.
vec3 applyFXAA(vec2 uv, bool useBloom, bool useGrid, bool useOutline)
{
  vec2 texelSize = 1.0 / vec2(textureSize(gDepth, 0).xy);
  vec4 offsetPos = vec4(texelSize, texelSize) * vec4(-1.0, 1.0, 1.0, -1.0);

  vec3 rgbNW = texture(screenColour, uv + vec2(-1.0, -1.0) * texelSize).rgb;
  vec3 rgbNE = texture(screenColour, uv + vec2(1.0, -1.0) * texelSize).rgb;
  vec3 rgbSW = texture(screenColour, uv + vec2(-1.0, 1.0) * texelSize).rgb;
  vec3 rgbSE = texture(screenColour, uv + vec2(1.0, 1.0) * texelSize).rgb;
  vec3 rgbM = texture(screenColour, uv).rgb;

  if (useBloom)
  {
    rgbNW += u_bloom.x * upsampleBoxTent(bloomColour, uv + vec2(-1.0, -1.0) * texelSize, u_bloom.y);
    rgbNE += u_bloom.x * upsampleBoxTent(bloomColour, uv + vec2(1.0, -1.0) * texelSize, u_bloom.y);
    rgbSW += u_bloom.x * upsampleBoxTent(bloomColour, uv + vec2(-1.0, 1.0) * texelSize, u_bloom.y);
    rgbSE += u_bloom.x * upsampleBoxTent(bloomColour, uv + vec2(1.0, 1.0) * texelSize, u_bloom.y);
    rgbM += u_bloom.x * upsampleBoxTent(bloomColour, uv, u_bloom.y).rgb;
  }

  if (useGrid)
  {
    rgbNW = applyGrid(rgbNW, gDepth, uv + vec2(-1.0, -1.0) * texelSize, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    rgbNE = applyGrid(rgbNE, gDepth, uv + vec2(1.0, -1.0) * texelSize, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    rgbSW = applyGrid(rgbSW, gDepth, uv + vec2(-1.0, 1.0) * texelSize, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    rgbSE = applyGrid(rgbSE, gDepth, uv + vec2(1.0, 1.0) * texelSize, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    rgbM = applyGrid(rgbM, gDepth, uv, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
  }

  if (useOutline)
  {
    rgbNW = applyOutline(rgbNW, gEntityIDMask, uv + vec2(-1.0, -1.0) * texelSize);
    rgbNE = applyOutline(rgbNE, gEntityIDMask, uv + vec2(1.0, -1.0) * texelSize);
    rgbSW = applyOutline(rgbSW, gEntityIDMask, uv + vec2(-1.0, 1.0) * texelSize);
    rgbSE = applyOutline(rgbSE, gEntityIDMask, uv + vec2(1.0, 1.0) * texelSize);
    rgbM = applyOutline(rgbM, gEntityIDMask, uv);
  }

  rgbNW = toneMap(rgbNW, uint(u_postSettings.y));
  rgbNE = toneMap(rgbNE, uint(u_postSettings.y));
  rgbSW = toneMap(rgbSW, uint(u_postSettings.y));
  rgbSE = toneMap(rgbSE, uint(u_postSettings.y));
  rgbM = toneMap(rgbM, uint(u_postSettings.y));

  rgbNW = applyGamma(rgbNW, u_nearFarGamma.z);
  rgbNE = applyGamma(rgbNE, u_nearFarGamma.z);
  rgbSW = applyGamma(rgbSW, u_nearFarGamma.z);
  rgbSE = applyGamma(rgbSE, u_nearFarGamma.z);
  rgbM = applyGamma(rgbM, u_nearFarGamma.z);

  float lumaNW = rgbToLuma(rgbNW);
  float lumaNE = rgbToLuma(rgbNE);
  float lumaSW = rgbToLuma(rgbSW);
  float lumaSE = rgbToLuma(rgbSE);
  float lumaM  = rgbToLuma(rgbM);

  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

  vec2 dir;
  dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
  dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

  float dirReduce = max(
      (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
      FXAA_REDUCE_MIN);

  float rcpDirMin = 1.0 / max(min(abs(dir.x), abs(dir.y)) + dirReduce, 1e-4);

  dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
        max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
        dir * rcpDirMin)) * texelSize;

  // Compute the blending factors.
  vec2 firstUV = uv + dir * (1.0 / 3.0 - 0.5);
  vec2 secondUV = uv + dir * (2.0 / 3.0 - 0.5);
  vec2 thirdUV = uv + dir * -0.5;
  vec2 fourthUV = uv + dir * 0.5;
  vec3 first = texture(screenColour, firstUV).rgb;
  vec3 second = texture(screenColour, secondUV).rgb;
  vec3 third = texture(screenColour, thirdUV).rgb;
  vec3 fourth = texture(screenColour, fourthUV).rgb;
  if (useBloom)
  {
    first += u_bloom.x * upsampleBoxTent(bloomColour, firstUV, u_bloom.y);
    second += u_bloom.x * upsampleBoxTent(bloomColour, secondUV, u_bloom.y);
    third += u_bloom.x * upsampleBoxTent(bloomColour, thirdUV, u_bloom.y);
    fourth += u_bloom.x * upsampleBoxTent(bloomColour, fourthUV, u_bloom.y);
  }
  if (useGrid)
  {
    first = applyGrid(first, gDepth, firstUV, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    second = applyGrid(second, gDepth, secondUV, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    third = applyGrid(third, gDepth, thirdUV, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
    fourth = applyGrid(fourth, gDepth, fourthUV, u_invViewProjMatrix, u_projMatrix * u_viewMatrix);
  }
  if (useOutline)
  {
    first = applyOutline(first, gEntityIDMask, firstUV);
    second = applyOutline(second, gEntityIDMask, secondUV);
    third = applyOutline(third, gEntityIDMask, thirdUV);
    fourth = applyOutline(fourth, gEntityIDMask, fourthUV);
  }
  first = toneMap(first, uint(u_postSettings.y));
  first = applyGamma(first, u_nearFarGamma.z);
  second = toneMap(second, uint(u_postSettings.y));
  second = applyGamma(second, u_nearFarGamma.z);
  third = toneMap(third, uint(u_postSettings.y));
  third = applyGamma(third, u_nearFarGamma.z);
  fourth = toneMap(fourth, uint(u_postSettings.y));
  fourth = applyGamma(fourth, u_nearFarGamma.z);
  vec3 rgbA = 0.5 * (max(first, vec3(0.0)) + max(second, vec3(0.0)));
  vec3 rgbB = 0.5 * rgbA + 0.25 * (max(third, vec3(0.0)) + max(fourth, vec3(0.0)));

  float lumaB = rgbToLuma(rgbB);
  if ((lumaB < lumaMin) || (lumaB > lumaMax))
    return rgbA;
  else
    return rgbB;
}

vec3 reinhardOperator(vec3 rgbColour)
{
  return rgbColour / (vec3(1.0) + rgbColour);
}

vec3 luminanceReinhardOperator(vec3 rgbColour)
{
  return rgbColour / (1.0 + rgbToLuma(rgbColour));
}

vec3 luminanceReinhardJodieOperator(vec3 rgbColour)
{
  float luminance = rgbToLuma(rgbColour);
  vec3 tv = rgbColour / (vec3(1.0) + rgbColour);

  return mix(rgbColour / (1.0 + luminance), tv, tv);
}

vec3 partialUnchartedOperator(vec3 rgbColour)
{
  const float a = 0.15;
  const float b = 0.50;
  const float c = 0.10;
  const float d = 0.20;
  const float e = 0.02;
  const float f = 0.30;

  return ((rgbColour * (a * rgbColour + c * b) + d * e)
          / (rgbColour * (a * rgbColour + b) + d * f)) - e / f;
}

vec3 filmicUnchartedOperator(vec3 rgbColour)
{
  float bias = 2.0;
  vec3 current = partialUnchartedOperator(bias * rgbColour);

  vec3 white = vec3(11.2);
  vec3 whiteScale = vec3(1.0) / partialUnchartedOperator(white);

  return current * whiteScale;
}

vec3 fastAcesOperator(vec3 rgbColour)
{
  rgbColour *= 0.6;
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;

  return clamp((rgbColour * (a * rgbColour + b))
               / (rgbColour * (c * rgbColour + d) + e), 0.0f, 1.0f);
}

vec3 acesOperator(vec3 rgbColour)
{
  const mat3 inputMatrix = mat3
  (
    vec3(0.59719, 0.07600, 0.02840),
    vec3(0.35458, 0.90834, 0.13383),
    vec3(0.04823, 0.01566, 0.83777)
  );

  const mat3 outputMatrix = mat3
  (
    vec3(1.60475, -0.10208, -0.00327),
    vec3(-0.53108, 1.10813, -0.07276),
    vec3(-0.07367, -0.00605, 1.07602)
  );

  vec3 inputColour = inputMatrix * rgbColour;
  vec3 a = inputColour * (inputColour + vec3(0.0245786)) - vec3(0.000090537);
  vec3 b = inputColour * (0.983729 * inputColour + 0.4329510) + 0.238081;
  vec3 c = a / b;
  return max(outputMatrix * c, 0.0.xxx);
}

// Tone mapping.
vec3 toneMap(vec3 colour, uint operator)
{
  switch (operator)
  {
    case 0: return reinhardOperator(colour);
    case 1: return luminanceReinhardOperator(colour);
    case 2: return luminanceReinhardJodieOperator(colour);
    case 3: return filmicUnchartedOperator(colour);
    case 4: return fastAcesOperator(colour);
    case 5: return acesOperator(colour);
    default: return colour;
  }
}

// Gamma correct.
vec3 applyGamma(vec3 colour, float gamma)
{
  return pow(colour, vec3(1.0 / gamma));
}

vec3 unProject(vec3 position, mat4 invVP)
{
  vec4 temp = invVP * vec4(position, 1.0);
  return temp.xyz / temp.w;
}

vec2 xzTransparency(vec3 xzFragPos3D, float scale)
{
  vec2 coord = xzFragPos3D.xz * scale;
  vec2 derivative = fwidth(coord);
  vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
  float line = min(grid.x, grid.y);

  float transparency = 1.0 - min(line, 1.0);
  return vec2(transparency * 0.4, line < 1.0);
}

vec4 grid(vec3 xzFragPos3D, float scale)
{
  vec2 coord = xzFragPos3D.xz * scale;
  vec2 derivative = fwidth(coord);

  float minimumz = min(derivative.y, 1.0);
  float minimumx = min(derivative.x, 1.0);

  vec2 transparency = xzTransparency(xzFragPos3D, scale);
  vec4 color = vec4(transparency.xxx, transparency.y);

  // Z axis.
  if (xzFragPos3D.x > -1.0 * minimumx && xzFragPos3D.x < 1.0 * minimumx)
    color.xy = 0.0.xx;
  // X axis.
  if (xzFragPos3D.z > -1.0 * minimumz && xzFragPos3D.z < 1.0 * minimumz)
    color.yz = 0.0.xx;

  return color;
}

// Apply the grid.
vec3 applyGrid(vec3 colour, sampler2D gDepth, vec2 uvs, mat4 invVP, mat4 vP)
{
  uvs = 2.0 * uvs - 1.0.xx;
  vec3 nearPoint = unProject(vec3(uvs, 0.0), invVP);
  vec3 farPoint = unProject(vec3(uvs, 1.0), invVP);
  float t = -nearPoint.y / (farPoint.y - nearPoint.y);
  float s = -nearPoint.z / (farPoint.z - nearPoint.z);

  vec3 xzFragPos3D = nearPoint + t * (farPoint - nearPoint);
  vec3 xyFragPos3D = nearPoint + s * (farPoint - nearPoint);

  // Compute the depth of the current fragment along both the x-y and x-z planes
  // [0, 1].
  vec4 xzFragClipPos = vP * vec4(xzFragPos3D, 1.0);
  float xzFragDepth = 0.5 * (xzFragClipPos.z / xzFragClipPos.w) + 0.5;

  // Perform a depth test so the grid doesn't draw over scene objects.
  float depth = texture(gDepth, 0.5 * uvs + 0.5.xx).r;
  float xzDepthTest = float(depth > xzFragDepth);

  // Apply the grid within 10 units.
  float aabb = float(abs(xzFragPos3D.x) <= 10.0 && abs(xzFragPos3D.z) <= 10.0);
  vec4 gridColour = grid(xzFragPos3D, 1.0) * float(t > 0.0);

  return mix(colour, gridColour.rgb, xzDepthTest * aabb * gridColour.a);
}

// Apply the outline.
vec3 applyOutline(vec3 colour, sampler2D idMask, vec2 uvs)
{
  vec2 texel = 1.0.xx / vec2(textureSize(idMask, 0).xy);

  // Populate the Sobel edge detection kernel.
  float kernel[9];
  kernel[0] = texture(idMask, uvs + vec2(-texel.x, -texel.y)).r;
	kernel[1] = texture(idMask, uvs + vec2(0.0, -texel.y)).r;
	kernel[2] = texture(idMask, uvs + vec2(texel.x, -texel.y)).r;
	kernel[3] = texture(idMask, uvs + vec2(-texel.x, 0.0)).r;
	kernel[4] = texture(idMask, uvs).r;
	kernel[5] = texture(idMask, uvs + vec2(texel.x, 0.0)).r;
	kernel[6] = texture(idMask, uvs + vec2(-texel.x, texel.y)).r;
	kernel[7] = texture(idMask, uvs + vec2(0.0, texel.y)).r;
	kernel[8] = texture(idMask, uvs + vec2(texel.x, texel.y)).r;

  // Find the edge of the entity mask.
  float sobelEdgeH = kernel[2] + (2.0 * kernel[5]) + kernel[8] - (kernel[0] + (2.0 * kernel[3]) + kernel[6]);
  float sobelEdgeV = kernel[0] + (2.0 * kernel[1]) + kernel[2] - (kernel[6] + (2.0 * kernel[7]) + kernel[8]);
  float sobel = sqrt((sobelEdgeH * sobelEdgeH) + (sobelEdgeV * sobelEdgeV));

  return mix(colour, vec3(sobel, 0.0, 0.0), float(sobel >= 1e-2));
}

// Iestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified for VR.
vec3 screenSpaceDither(vec2 uvs) 
{
	vec3 dither = vec3(dot(vec2(171.0, 231.0), uvs));
	dither.rgb = fract(dither.rgb / vec3(103.0, 71.0, 97.0));
	// Subtract 0.5 to avoid slightly brightening the whole viewport.
	return (dither.rgb - 0.5) / 255.0;
}