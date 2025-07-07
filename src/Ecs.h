#pragma once

#include <HECS/World.h>

class Ecs {
public:
    static Hori::World& GetInstance() {
        static Ecs instance;
        return instance.m_world;
    }

private:
    Ecs() = default;
    ~Ecs() = default;

    Hori::World m_world; 
};