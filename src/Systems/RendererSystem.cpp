#include "RendererSystem.h"
#include "../Components/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../Components/RayTagged.h"
#include "../ImGuiUtils.h"

RenderSystem::RenderSystem(Renderer& renderer)
    : m_renderer(&renderer)
{}

void RenderSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();
    Camera camera;
    ecs.Each<Camera>([&camera](Hori::Entity e, const Camera& cam) {
        camera = cam;
    });

    auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
    sceneData->proj = camera.projection;
    sceneData->view = camera.view;
    sceneData-> viewproj = camera.viewProjection;

    m_renderer->BeginRendering();
    renderDrawables();
    renderGui();
    m_renderer->EndRendering();
}

void RenderSystem::renderDrawables()
{
    auto& ecs = Ecs::GetInstance();
    std::vector<RenderObject> objects;
    ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity, Drawable& drawable, LocalToWorld& localToWorld) {
        for (auto& [startIndex, count, material] : drawable.mesh->surfaces)
        {
            RenderObject def;
            def.indexCount = count;
            def.firstIndex = startIndex;
            def.indexBuffer = drawable.mesh->meshBuffers->indexBuffer.buffer;
            def.material = material.get();
            def.transform = localToWorld.value;
            def.vertexBufferAddress = drawable.mesh->meshBuffers->vertexBufferAddress;

            objects.push_back(def);
        }
    });

    m_renderer->RenderObjects(objects);
}

void RenderSystem::renderGui()
{
    auto& ecs = Ecs::GetInstance();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Object info");
    ecs.Each<RayTagged>([&ecs](Hori::Entity e, RayTagged) {
        if (ecs.HasComponents<Translation>(e))
            ImGui::InputFloat3("Translation", glm::value_ptr(ecs.GetComponent<Translation>(e)->value));

        if (ecs.HasComponents<Rotation>(e))
            ImGui::InputFloat4("Rotation", glm::value_ptr(ecs.GetComponent<Rotation>(e)->value));

        if (ecs.HasComponents<Translation>(e))
            ImGui::InputFloat3("Scale", glm::value_ptr(ecs.GetComponent<Scale>(e)->value));
    });
    ImGui::End();

    ImGui::Render();
    m_renderer->RenderImGui();
}
