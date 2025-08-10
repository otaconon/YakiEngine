#pragma once

#include <glm/glm.hpp>

struct PointLight
{
    glm::vec4 color;
    glm::vec3 position;
    float rMin;
    float rMax;
};
