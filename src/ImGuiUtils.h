#pragma once

#include <imgui.h>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ImGuiUtils
{
    inline void InputVec3(glm::vec3& v, const std::string& label)
    {
    }

    inline void InputQuat(glm::quat& q)
    {
        ImGui::InputFloat("x", &q.x);
        ImGui::SameLine();
        ImGui::InputFloat("y", &q.y);
        ImGui::SameLine();
        ImGui::InputFloat("z", &q.z);
        ImGui::SameLine();
        ImGui::InputFloat("w", &q.w);
    }


}
