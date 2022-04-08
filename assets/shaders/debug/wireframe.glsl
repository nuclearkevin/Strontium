#type common
#version 460 core
/*
 * A wireframe shader for debugging and visualizing objects.
 */

struct WireframeData
{
  mat4 transform;
  vec4 colour;
};

struct VertexData
{
  vec4 normal;
  vec4 tangent;
  vec4 position; // Uncompressed position (x, y, z). w is padding.
  vec4 boneWeights; // Uncompressed bone weights.
  ivec4 boneIDs; // Bone IDs.
  vec4 texCoord; // UV coordinates (x, y). z and w are padding.
};

#type vertex
// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

layout(std140, binding = 0) readonly buffer VertexBuffer
{
  VertexData v_vertices[];
};

layout(std430, binding = 1) readonly buffer IndexBuffer
{
  uint v_indices[];
};

layout(std140, binding = 2) readonly buffer ModelBlock
{
  WireframeData u_data[];
};

// Vertex properties for shading.
out VERT_OUT
{
  vec4 fColour;
} vertOut;

void main()
{
  const uint vIndex = v_indices[gl_VertexID];
  const VertexData vertex = v_vertices[vIndex];

  const int instance = gl_InstanceID;
  const mat4 modelMatrix = u_data[instance].transform;

  gl_Position = u_projMatrix * u_viewMatrix * modelMatrix * vec4(vertex.position.xyz, 1.0);

  vertOut.fColour = u_data[instance].colour;
}

#type fragment
layout(location = 0) out vec4 gColour;

in VERT_OUT
{
	vec4 fColour;
} fragIn;

void main()
{
  gColour = fragIn.fColour;
}
