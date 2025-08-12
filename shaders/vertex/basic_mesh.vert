#version 450
#extension GL_EXT_buffer_reference : require

const int MAX_DIRECTIONAL_LIGHTS = 1;
const int MAX_PUNCTUAL_LIGHTS = 1;

struct Vertex {

    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 texCoord;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
    Vertex vertices[];
};

layout(push_constant) uniform constants
{
    mat4 model;
    mat4 view;
    mat4 proj;
    VertexBuffer vertexBuffer;
} PushConstants;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = PushConstants.proj * PushConstants.view * PushConstants.model * vec4(v.position, 1.0);
    fragColor = v.color;
    fragTexCoord = v.texCoord;
}
