#version 450

#extension GL_GOOGLE_include_directive : require
#include "../shared/input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 vPosition;

layout (location = 0) out vec4 outFragColor;

vec3 lit(vec3 l, vec3 n, vec3 v) {
    vec3 r_l = reflect(-l, n);
    float s = clamp(100.0 * dot(r_l, v) - 97.0, 0.0, 1.0);
    vec3 highlightColor = vec3(2, 2, 2);
    return mix(vec3(0.6, 0.2, 0.2), highlightColor, s);
}

void main()
{
    vec3 n = normalize(inNormal);
    vec3 v = normalize(sceneData.eyePosition.xyz - vPosition);

    vec3 color = inColor * texture(colorTex, inUV).xyz;
    vec3 ambient = color * sceneData.ambientColor.xyz;

    outFragColor = vec4(0.f, 0.f, 0.f, 1.0f);
    for (uint i = 0u; i < MAX_POINT; i++) {
        vec3 l = normalize(lightBuffer.pointLights[i].position.xyz - vPosition);
        float NdL = clamp(dot(n, l), 0.0f, 1.0f);
        outFragColor.rgb += NdL * lightBuffer.pointLights[i].color.rgb * lit(l, n, v);
    }
    outFragColor.xyz *= color;
    outFragColor.xyz += ambient;
}