#pragma once

#include <World.h>

class Ecs {
public:
    static Hori::World& GetInstance() {
        static Ecs instance;
        return instance.m_world;
    }

private:
    Ecs() = default;
    ~Ecs() = default;
    Ecs(const Ecs&) = default;
    Ecs& operator=(const Ecs&) = default;

    Hori::World m_world; 
};