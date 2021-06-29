#version 440

uniform vec2 screenSize;
uniform sampler2D entity;

layout(location = 1) out vec4 fragColour;

void main()
{
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
