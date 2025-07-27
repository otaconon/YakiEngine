#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {

    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 texCoord;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
    Vertex vertices[];
};

layout( push_constant ) uniform constants
{
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} PushConstants;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = PushConstants.render_matrix * vec4(v.position, 1.0);
    fragColor = v.color;
    fragTexCoord = v.texCoord;
}
