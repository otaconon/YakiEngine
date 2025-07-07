#include "ControllerSystem.h"

#include <SDL3/SDL.h>

#include "../InputData.h"
#include "../Components/Controller.h"

void ControllerSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.Each<Controller>([&ecs](Hori::Entity, Controller& controller) {
        controller.dx = 0.f;
        controller.dy = 0.f;

        SDL_GetMouseState(&controller.mouseX, &controller.mouseY);

        if (controller.mouseMode == MouseMode::EDITOR)
            return;

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

        for (auto& motion : ecs.GetSingletonComponent<InputEvents>()->mouseMotion)
        {
            controller.dx = motion.xrel;
            controller.dy = motion.yrel;
        }

        if (glm::length(dir) > 1e-6f)
            dir = glm::normalize(dir);

        controller.direction = dir;


    });
}
