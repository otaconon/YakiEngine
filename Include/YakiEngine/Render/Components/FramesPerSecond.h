#pragma once

#include <cstdint>

struct FramesPerSecond
{
    uint32_t frameCount{0};
    float timer{0.f};
    float value{0.f};
};