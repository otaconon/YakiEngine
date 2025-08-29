#pragma once
#include <functional>
#include <imgui.h>
#include <span>
#include <string_view>
#include <vector>

template<typename ItemType>
class ItemList
{
public:
    ItemList(std::function<void(ItemType&)> displayFunction)
        : m_displayFunction{displayFunction}
    {}

    void Begin(const std::string& label, std::vector<std::string> columns)
    {
        ImGui::Begin(label.c_str());
        m_currentId = 0;
    }
    void AddItem(ItemType& item, const std::string& label = "")
    {
        if (ImGui::TreeNode(label.c_str()))
        {
            m_displayFunction(item);
            ImGui::TreePop();
        }
    }
    void End()
    {
        ImGui::End();
    }

private:
    std::function<void(ItemType&)> m_displayFunction;
    uint32_t m_currentId{0};
};
