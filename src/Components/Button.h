#pragma once

#include <functional>

struct Button
{
    std::string label;
    std::function<void()> onClick;
};