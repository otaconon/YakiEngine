#pragma once

#include <glm/glm.hpp>

#include "Components.h"

struct Camera {
    float near{0.1f};
    float far{100.f};
    float fovx{90.f};
    glm::ivec2 aspectRatio{1600, 900};
    bool isPerspective{true};

    glm::mat4 projection{};
    glm::mat4 view{};
    glm::mat4 viewProjection{};

    float orthoWidth{10.f};
    float orthoHeight{10.f};
};
