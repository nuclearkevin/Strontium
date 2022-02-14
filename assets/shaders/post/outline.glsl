#type common
#version 460 core
/*
 * An outline post-processing shader for the Strontium editor.
 */

#type vertex
void main()
{
  vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;

  gl_Position = vec4(position, 0.0, 1.0);
}

#type fragment
layout(binding = 0) uniform sampler2D entity;

// The post processing properties.
layout(std140, binding = 1) uniform PostProcessBlock
{
  vec4 u_camPosScreenSize; // Camera position (x, y, z) and the screen width (w).
  vec4 u_screenSizeGammaBloom;  // Screen height (x), gamma (y) and bloom intensity (z). w is unused.
  ivec4 u_postProcessingPasses; // Tone mapping operator (x), using bloom (y), using FXAA (z) and using sunshafts (w).
};

layout(location = 0) out vec4 fragColour;

void main()
{
  vec2 screenSize = vec2(textureSize(entity, 0).xy);
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;

  float kernel[9];
  float w = 1.0 / screenSize.x;
  float h = 1.0 / screenSize.y;

  kernel[0] = texture2D(entity, fTexCoords + vec2(-w, -h)).r;
	kernel[1] = texture2D(entity, fTexCoords + vec2(0.0, -h)).r;
	kernel[2] = texture2D(entity, fTexCoords + vec2(w, -h)).r;
	kernel[3] = texture2D(entity, fTexCoords + vec2(-w, 0.0)).r;
	kernel[4] = texture2D(entity, fTexCoords).r;
	kernel[5] = texture2D(entity, fTexCoords + vec2(w, 0.0)).r;
	kernel[6] = texture2D(entity, fTexCoords + vec2(-w, h)).r;
	kernel[7] = texture2D(entity, fTexCoords + vec2(0.0, h)).r;
	kernel[8] = texture2D(entity, fTexCoords + vec2(w, h)).r;

  float sobelEdgeH = kernel[2] + (2.0 * kernel[5]) + kernel[8] - (kernel[0] + (2.0 * kernel[3]) + kernel[6]);
  float sobelEdgeV = kernel[0] + (2.0 * kernel[1]) + kernel[2] - (kernel[6] + (2.0 * kernel[7]) + kernel[8]);
  float sobel = sqrt((sobelEdgeH * sobelEdgeH) + (sobelEdgeV * sobelEdgeV));

  fragColour = vec4(vec3(sobel) * vec3(1.8, 0.0, 0.0), 1.0);
}
