#pragma once

#include <SDL3/SDL.h>

struct InputEvents {
    std::vector<SDL_MouseMotionEvent> mouseMotion;
    std::vector<SDL_MouseButtonEvent> mouseButton;
    std::vector<SDL_KeyboardEvent> keyDown;
    std::vector<SDL_QuitEvent> quit;

    void Clear()
    {
        mouseMotion.clear();
        mouseButton.clear();
        keyDown.clear();
        quit.clear();
    }
};