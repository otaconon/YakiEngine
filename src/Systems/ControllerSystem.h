#pragma once

#include <SDL3/SDL_video.h>

#include "../Ecs.h"

class ControllerSystem : public Hori::System
{
public:
    ControllerSystem(SDL_Window* window);

    void Update(float dt) override;

private:
    SDL_Window* m_window;

};
