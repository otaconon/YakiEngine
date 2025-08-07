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
#include "../Components/DirectionalLight.h"

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

    // Assumes there is only one light of each type in the scene (idk how to handle more yet)
    ecs.Each<DirectionalLight>([&ecs](Hori::Entity, DirectionalLight& light) {
        auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
        sceneData->sunlightColor = light.color;
        sceneData->sunlightDirection = glm::vec4(light.direction, 1.f);
    });

    ecs.Each<AmbientLight>([&ecs](Hori::Entity, AmbientLight& light) {
       auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
        sceneData->ambientColor = light.color;
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

    ImGui::Begin("Light info");
    ecs.Each<DirectionalLight>([](Hori::Entity, DirectionalLight& light) {
        ImGui::PushID("Directional light");
        ImGui::Text("Directional light");
        ImGui::InputFloat4("Color", glm::value_ptr(light.color));
        ImGui::InputFloat3("Direction", glm::value_ptr(light.direction));
        ImGui::PopID();
    });
    ecs.Each<AmbientLight>([](Hori::Entity, AmbientLight& light) {
        ImGui::PushID("Ambient light");
        ImGui::Text("Ambient light");
        ImGui::InputFloat4("Color", glm::value_ptr(light.color));
        ImGui::PopID();
    });
    ImGui::End();

    ImGui::Begin("Object info");
    ecs.Each<RayTagged>([&ecs](Hori::Entity e, RayTagged) {
        if (ecs.GetComponentArray<RayTagged>().Size() > 1)
        {
            ecs.RemoveComponents<RayTagged>(e);
            return;
        }
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
