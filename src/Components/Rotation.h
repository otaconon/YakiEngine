#pragma once

#include <glm/detail/type_quat.hpp>

struct Rotation
{
    glm::quat value{0.f, 0.f, 0.f, 1.f};
    float roll{}, pitch{}, yaw{};
};
