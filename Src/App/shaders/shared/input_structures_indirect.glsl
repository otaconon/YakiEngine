#define MAX_DIRECTIONAL 10
#define MAX_POINT 10

struct DirectionalLight {
    vec4 color;
    vec4 direction;
};

struct PointLight {
    vec4 color;
    vec4 position;
};

struct MaterialParams {
    vec4 colorFactors;
    vec4 metalRoughFactors;
    vec4 specularColorFactors;
};

layout(set = 0, binding = 0) uniform  SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 eyePosition;
} sceneData;

layout (set = 0, binding = 1, std430) readonly buffer LightBuffer {
    uint numDirectionalLights;
    uint numPointLights;
    uvec2 padding;
    DirectionalLight directionalLights[MAX_DIRECTIONAL];
    PointLight pointLights[MAX_POINT];
} lightBuffer;