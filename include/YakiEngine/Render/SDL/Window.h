#pragma once

#include <SDL3/SDL.h>

class Window {
public:
	Window();
	~Window();

	[[nodiscard]] SDL_Window* window();

private:
    SDL_Window* m_window;

	static bool resizingEventWatcher(void* data, SDL_Event* event);
};