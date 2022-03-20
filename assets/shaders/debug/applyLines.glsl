#type common
#version 460 core

#type vertex
void main()
{
  vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;

  gl_Position = vec4(position, 0.0, 1.0);
}

#type fragment
layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D lColour;
layout(binding = 2) uniform sampler2D lDepth;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

void main()
{
  vec2 screenSize = vec2(textureSize(lColour, 0).xy);
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;

  // Depth test the lines.
  float line = texture(lDepth, fTexCoords).r;
  float geo = texture(gDepth, fTexCoords).r;
  float alpha = line < geo ? 1.0 : 0.0;

  fragColour = vec4(texture(lColour, fTexCoords).rgb, alpha);
}
