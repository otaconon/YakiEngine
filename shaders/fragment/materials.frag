#version 460

#extension GL_GOOGLE_include_directive : require

#include "../shared/input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 vPosition;
layout (location = 4) in flat uint inObjectId;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out uint outObjectId;

vec3 lit(vec3 l, vec3 n, vec3 v) {
    vec3 r_l = reflect(-l, n);
    float s = clamp(100.0 * dot(r_l, v) - 97.0, 0.0, 1.0);
    vec3 highlightColor = (materialData.specular_color_factors.xyz * materialData.specular_color_factors.w);
    return highlightColor * s;
}

void main()
{
    vec3 n = normalize(inNormal);
    vec3 v = normalize(sceneData.eyePosition.xyz - vPosition);

    vec3 color = inColor * texture(colorTex, inUV).xyz;
    vec3 ambient = color * (sceneData.ambientColor.xyz  * sceneData.ambientColor.w);

    outFragColor = vec4(0.f, 0.f, 0.f, 1.0f);
    for (uint i = 0u; i < lightBuffer.numPointLights; i++) {
        vec3 lightPos = lightBuffer.pointLights[i].position.xyz;
        vec3 l = normalize(lightPos - vPosition);
        float NdL = clamp(dot(n, l), 0.0f, 1.0f);

        vec3 diffuse = NdL * lightBuffer.pointLights[i].color.rgb * color;
        vec3 specular = lightBuffer.pointLights[i].color.rgb * lit(l, n, v);

        outFragColor.rgb += diffuse + specular;
    }


    outFragColor += vec4(ambient, 1.0);
    outObjectId = inObjectId;

    // Render normals
    /*
    vec3 normalColor = n * 0.5 + 0.5;
    outFragColor = vec4(normalColor, 1.0);
    */
}