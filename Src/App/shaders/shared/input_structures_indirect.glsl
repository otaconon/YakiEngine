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

layout(set = 1, binding = 0) uniform GLTFMaterialData{
    vec4 colorFactors;
    vec4 metal_rough_factors;
    vec4 specular_color_factors;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;