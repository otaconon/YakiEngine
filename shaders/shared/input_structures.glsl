const int MAX_DIRECTIONAL = 1;
const int MAX_POINT = 1;

struct DirectionalLight {
    vec4 color;
    vec3 direction;
};

struct PointLight {
    vec4 color;
    vec3 position;
    float rMin;
    float rMax;};

layout(set = 0, binding = 0) uniform  SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec3 eyePosition;
} sceneData;

layout (std430, set = 0, binding = 1) restrict readonly buffer LightBuffer {
    DirectionalLight directionalLights[MAX_DIRECTIONAL];
    PointLight pointLights[MAX_POINT];
} lightBuffer;

layout(set = 1, binding = 0) uniform GLTFMaterialData{
    vec4 colorFactors;
    vec4 metal_rough_factors;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;