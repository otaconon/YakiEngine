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
    auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
    Camera camera;
    ecs.Each<Camera, Translation>([&camera, &sceneData](Hori::Entity, Camera& cam, Translation& pos) {
        camera = cam;
        sceneData->eyePos = glm::vec4(pos.value, 1.0f);
    });
    sceneData->proj = camera.projection;
    sceneData->view = camera.view;
    sceneData->viewproj = camera.viewProjection;

    m_renderer->BeginRendering();
    m_renderer->Begin3DRendering();
    renderDrawables();
    renderColliders();
    m_renderer->End3DRendering();
    renderGui();
    m_renderer->EndRendering();
}

void RenderSystem::renderDrawables()
{
    auto& ecs = Ecs::GetInstance();
    std::vector<RenderObject> objects;
    ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity e, Drawable& drawable, LocalToWorld& localToWorld) {
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

    ImGui::Begin("SceneData");
    auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
    ImGui::InputFloat4("Ambient color", glm::value_ptr(sceneData->ambientColor));
    ImGui::End();

    uint32_t id = 0;
    ImGui::Begin("Light info");
    ecs.Each<DirectionalLight>([&id](Hori::Entity, DirectionalLight& light) {
        ImGui::PushID(std::format("Directional light##{}", id++).c_str());
        ImGui::Text("Directional light");
        ImGui::InputFloat4("Color", glm::value_ptr(light.color));
        ImGui::InputFloat3("Direction", glm::value_ptr(light.direction));
        ImGui::PopID();
    });
    ecs.Each<PointLight>([&id](Hori::Entity, PointLight& light) {
        ImGui::PushID(std::format("Point light##{}", id++).c_str());
        ImGui::Text("Point light");
        ImGui::InputFloat4("Color", glm::value_ptr(light.color));
        ImGui::PopID();
   });
    ImGui::End();

    ImGui::Begin("Object info");
    ecs.Each<RayTagged, Translation, Rotation, Scale, Property<Translation>, Property<Rotation>, Property<Scale>>
    ([&ecs](Hori::Entity e, RayTagged, Translation& translation, Rotation& rotation, Scale& scale, Property<Translation>& pTranslation, Property<Rotation>& pRotation, Property<Scale>& pScale) {
        if (ecs.GetComponentArray<RayTagged>().Size() > 1)
        {
            ecs.RemoveComponents<RayTagged>(e);
            return;
        }

        ImGui::InputFloat3(pTranslation.label.c_str(), glm::value_ptr(translation.value));
        ImGui::InputFloat3(pRotation.label.c_str(), glm::value_ptr(rotation.value));
        ImGui::InputFloat3(pScale.label.c_str(), glm::value_ptr(scale.value));
    });
    ImGui::End();

    ImGui::Render();
    m_renderer->RenderImGui();
}

void RenderSystem::renderColliders()
{
    auto& ecs = Ecs::GetInstance();
    std::vector<WireframeObject> objects;
    ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity e, Drawable& drawable, LocalToWorld& localToWorld) {
        for (auto& [startIndex, count, material] : drawable.mesh->surfaces)
        {
            WireframeObject def;
            def.indexCount = count;
            def.firstIndex = startIndex;
            def.indexBuffer = drawable.mesh->meshBuffers->indexBuffer.buffer;
            def.transform = localToWorld.value;
            def.vertexBufferAddress = drawable.mesh->meshBuffers->vertexBufferAddress;

            objects.push_back(def);
        }
    });

    m_renderer->RenderWireframes(objects);
}
