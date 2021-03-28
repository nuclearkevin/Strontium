#version 440

void main()
{
  gl_FragDepth = gl_FragCoord.z;
}
