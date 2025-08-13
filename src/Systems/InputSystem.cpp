#include "InputSystem.h"

#include <SDL3/SDL_events.h>

#include "../Ecs.h"
#include "../Components/Components.h"
#include "../InputData.h"

InputSystem::InputSystem(SDL_Window* window)
    : m_window{window}
{}

void InputSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.Each<Controller>([this](Hori::Entity, Controller& controller) {
        processKeyboardEvents(controller);
        processMouseEvents(controller);
    });

    Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_KeyboardEvent>>()->queue.clear();
    Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_MouseButtonEvent>>()->queue.clear();
}

void InputSystem::processKeyboardEvents(Controller& controller)
{
    for (auto& event : Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_KeyboardEvent>>()->queue)
    {
        if (event.key == SDLK_ESCAPE)
        {
            SDL_SetWindowRelativeMouseMode(m_window, false);
            SDL_CaptureMouse(false);
            controller.mouseMode = MouseMode::EDITOR;
        }
        else if (event.key == SDLK_SPACE)
        {
            SDL_SetWindowRelativeMouseMode(m_window, true);
            SDL_CaptureMouse(true);
            controller.mouseMode = MouseMode::GAME;
        }
    }
}

void InputSystem::processMouseEvents(Controller& controller)
{
    for (auto& event : Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_MouseButtonEvent>>()->queue)
    {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button == SDL_BUTTON_RIGHT)
        {
            if (controller.mouseMode == MouseMode::EDITOR && event.button == SDL_BUTTON_RIGHT)
            {
                SDL_SetWindowRelativeMouseMode(m_window, true);
                SDL_CaptureMouse(true);
                controller.mouseMode = MouseMode::EDITOR_CAMERA;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button == SDL_BUTTON_RIGHT) {
            SDL_SetWindowRelativeMouseMode(m_window, false);
            SDL_CaptureMouse(false);
            controller.mouseMode = MouseMode::EDITOR;
        }
        Ecs::GetInstance().GetSingletonComponent<InputEvents>()->mouseButton.push_back(event);
    }
}
