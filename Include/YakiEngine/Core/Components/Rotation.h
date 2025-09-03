#pragma once

#include <glm/glm.hpp>

struct Rotation
{
    glm::quat value{1.f, 0.f, 0.f, 0.f};
    float roll{0.f}, pitch{0.f}, yaw{0.f};
};
