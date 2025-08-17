#pragma once

#include <HECS/System.h>
#include <SDL3/SDL_video.h>

#include "../Components/Components.h"

class InputSystem : public Hori::System
{
public:
    InputSystem(SDL_Window* window);

    void Update(float dt) override;

private:
    SDL_Window* m_window;

    void processKeyboardEvents(Controller& controller);
    void processMouseButtonEvents(Controller& controller);
    void processMouseMotionEvents(Controller& controller);
    void processKeyboardState(Controller& controller);
};
