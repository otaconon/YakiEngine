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
        {
            ImGui::Text("Translation");
            glm::vec3& translation = ecs.GetComponent<Translation>(e)->value;
            ImGui::InputFloat("x##translation.x", &translation.x);
            ImGui::InputFloat("y##translation.y", &translation.y);
            ImGui::InputFloat("z##translation.z", &translation.z);
        }
        if (ecs.HasComponents<Rotation>(e))
        {
            ImGui::Text("Rotation");
            glm::quat& rotation = ecs.GetComponent<Rotation>(e)->value;
            ImGui::InputFloat("x##rotation.x", &rotation.x);
            ImGui::InputFloat("y##rotation.y", &rotation.y);
            ImGui::InputFloat("z##rotation.z", &rotation.z);
            ImGui::InputFloat("w##rotation.w", &rotation.w);
        }
        if (ecs.HasComponents<Translation>(e))
        {
            ImGui::Text("Scale");
            glm::vec3& scale = ecs.GetComponent<Scale>(e)->value;
            ImGui::InputFloat("x##scale.x", &scale.x);
            ImGui::InputFloat("y##scale.y", &scale.y);
            ImGui::InputFloat("z##scale.z", &scale.z);
        }
    });
    ImGui::End();

    ImGui::Render();
    m_renderer->RenderImGui();
}
