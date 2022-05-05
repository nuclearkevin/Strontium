#type common
#version 460 core
/*
 * A static mesh shader program for the geometry pass.
 */

struct MaterialData
{
  vec4 mRAE; // Metallic (r), roughness (g), AO (b) and emission (a);
  vec4 albedoReflectance; // Albedo (r, g, b) and reflectance (a);
};

struct EntityData
{
  mat4 u_transform;
  vec4 u_maskID; // Is this entity selected (x), and the entity ID (y). Z and w are unused.
  MaterialData u_material;
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

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

#type vertex
// An index for fetching data from the transform and editor SSBOs.
layout(std140, binding = 1) uniform PerDrawBlock
{
  int u_drawData; // Transform ID (x). Y, z and w are unused.
};

layout(std140, binding = 0) readonly buffer VertexBuffer
{
  VertexData v_vertices[];
};

layout(std430, binding = 1) readonly buffer IndexBuffer
{
  uint v_indices[];
};

// The per-entity data.
layout(std140, binding = 2) readonly buffer EntityBlock
{
  EntityData u_entityData[];
};

// Vertex properties for shading.
out VERT_OUT
{
  vec2 fTexCoords;
  vec2 fMaskID;
} vertOut;

void main()
{
  const uint vIndex = v_indices[gl_VertexID];
  const VertexData vertex = v_vertices[vIndex];

  // Compute the index of this draw into the global buffer.
  const int instance = gl_InstanceID + u_drawData;

  // Fetch the transform from the global buffer.
  const mat4 modelMatrix = u_entityData[instance].u_transform;

  gl_Position = u_projMatrix * u_viewMatrix * modelMatrix * vec4(vertex.position.xyz, 1.0);
  vertOut.fTexCoords = vertex.texCoord.xy;
  vertOut.fMaskID = u_entityData[instance].u_maskID.xy;
}

#type fragment
layout(location = 0) out vec2 gMaskID;

in VERT_OUT
{
  vec2 fTexCoords;
  vec2 fMaskID;
} fragIn;

layout(binding = 0) uniform sampler2D albedoMap;

void main()
{
  vec4 albedo = texture(albedoMap, fragIn.fTexCoords);
  if (albedo.a < 1e-4)
    discard;

  gMaskID = fragIn.fMaskID;
}
