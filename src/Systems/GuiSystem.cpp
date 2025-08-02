#include "GuiSystem.h"

#include "../Ecs.h"
#include "../Components/Button.h"

#include <imgui.h>

GuiSystem::GuiSystem() = default;

void GuiSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ImGui::Begin("Editor");
    ecs.Each<Button>([](Hori::Entity, Button& button) {
        if (ImGui::Button((button.label.c_str())))
            button.onClick();
    });
    ImGui::End();
}
