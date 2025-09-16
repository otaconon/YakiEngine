#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "../shared/input_structures_indirect.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 vPosition;
layout (location = 4) out flat uint outObjectId;

struct Vertex {
  vec3  position;
  float uv_x;
  vec3  normal;
  float uv_y;
  vec4  color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer { 
  Vertex vertices[]; 
};

layout(std430, set = 0, binding = 2) readonly buffer CompactedInstances {
  uint objectId[];
};

layout(std430, set = 0, binding = 3) readonly buffer ObjectData {
  mat4 model[];
};

layout(push_constant) uniform PC {
  VertexBuffer vertexBuffer;
} pc;

void main()
{
  uint globalInstance = gl_BaseInstance + gl_InstanceIndex;
  uint obj = objectId[globalInstance];
  mat4 M = model[obj];

  Vertex v = pc.vertexBuffer.vertices[gl_VertexIndex];

  vec4 position = vec4(v.position, 1.0);

  gl_Position = sceneData.viewproj * M * position;

  outNormal   = normalize((M * vec4(v.normal, 0.0)).xyz);
  outColor    = v.color.xyz * materialData.colorFactors.xyz;
  outUV       = vec2(v.uv_x, v.uv_y);
  vPosition   = (M * position).xyz;
  outObjectId = obj;
}