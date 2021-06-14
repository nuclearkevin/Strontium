#version 440

layout(location = 0) out vec4 fragColour;

in vec2 fTexCoords;

uniform sampler2D screenTexture;

void main()
{
    fragColour = texture(screenTexture, fTexCoords);
}



