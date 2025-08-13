#include "InputSystem.h"

#include <SDL3/SDL_events.h>

#include "../Ecs.h"
#include "../Components/Components.h"

InputSystem::InputSystem(SDL_Window* window)
    : m_window{window}
{}

void InputSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.Each<Controller>([this](Hori::Entity, Controller& controller) {
        controller.direction = {};
        controller.mouseButtonLeftPressed = false;
        controller.mouseButtonRightPressed = false;
        controller.dx = 0.f;
        controller.dy = 0.f;
        processKeyboardEvents(controller);
        processMouseButtonEvents(controller);
        processMouseMotionEvents(controller);
        processKeyboardState(controller);
    });

    Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_KeyboardEvent>>()->queue.clear();
    Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_MouseButtonEvent>>()->queue.clear();
    Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_MouseMotionEvent>>()->queue.clear();
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
            int width, height;
            SDL_GetWindowSize(m_window, &width, &height);
            SDL_WarpMouseInWindow(m_window, width/2, height/2); // If in game mode, center the mouse on the screen
            controller.mouseMode = MouseMode::GAME;
        }
    }
}

void InputSystem::processMouseButtonEvents(Controller& controller)
{
    for (auto& event : Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_MouseButtonEvent>>()->queue)
    {
        if (event.button == SDL_BUTTON_LEFT)
        {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                controller.mouseButtonLeftPressed = true;
            else
                controller.mouseButtonLeftPressed = false;
        }
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
    }
}

void InputSystem::processMouseMotionEvents(Controller& controller)
{
    for (auto& event : Ecs::GetInstance().GetSingletonComponent<InputQueue<SDL_MouseMotionEvent>>()->queue)
    {
        controller.direction = {};
        controller.dx = 0.f;
        controller.dy = 0.f;

        SDL_GetMouseState(&controller.mouseX, &controller.mouseY);
        if (controller.mouseMode == MouseMode::EDITOR)
        {
            controller.lockX = controller.mouseX;
            controller.lockY = controller.mouseY;
            return;
        }

        controller.dx = event.xrel;
        controller.dy = event.yrel;

        SDL_WarpMouseInWindow(m_window, controller.lockX, controller.lockY); // Make sure the mouse doesn't move when rotating camera
    }
}

void InputSystem::processKeyboardState(Controller& controller)
{
    glm::vec3 dir{};
    const auto* keyboard = SDL_GetKeyboardState(nullptr);
    if (keyboard[SDL_SCANCODE_W])
        dir.z -= 1.f;
    if (keyboard[SDL_SCANCODE_S])
        dir.z += 1.f;
    if (keyboard[SDL_SCANCODE_A])
        dir.x -= 1.f;
    if (keyboard[SDL_SCANCODE_D])
        dir.x += 1.f;

    if (glm::length(dir) > 1e-6f)
        dir = glm::normalize(dir);

    controller.direction = dir;
}
