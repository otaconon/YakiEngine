#include "RendererSystem.h"
#include "../Components/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <numeric>
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
    renderDrawables(camera.viewProjection);
    renderColliders();
    m_renderer->End3DRendering();
    renderGui(dt);
    m_renderer->EndRendering();

    Hori::Entity selectedEntity{m_renderer->GetHoveredEntityId()};
    if (selectedEntity.Valid())
        ecs.AddComponents(selectedEntity, Hovered{});
}

void RenderSystem::renderDrawables(const glm::mat4& viewproj)
{
    auto& ecs = Ecs::GetInstance();

    size_t drawableCount = ecs.GetComponentArray<Drawable>().Size();
    std::vector<RenderObject> objects;
    objects.reserve(drawableCount);

    ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity e, Drawable& drawable, LocalToWorld& localToWorld) {
        for (auto& [startIndex, count, bounds, material] : drawable.mesh->surfaces)
        {
            RenderObject obj {
                .objectId = e.id,
                .indexCount = count,
                .firstIndex = startIndex,
                .indexBuffer = drawable.mesh->meshBuffers->indexBuffer.buffer,
                .material = material.get(),
                .bounds = bounds,
                .transform = localToWorld.value,
                .vertexBufferAddress = drawable.mesh->meshBuffers->vertexBufferAddress
            };

            if (isVisible(obj, viewproj))
                objects.push_back(obj);
        }
    });

    std::vector<size_t> order(objects.size());
    std::iota(order.begin(), order.end(), 0);

    std::ranges::sort(order, [&objects](const auto& iA, const auto& iB) {
        auto& A = objects[iA];
        auto& B = objects[iB];
        if (A.material == B.material)
            return A.indexBuffer < B.indexBuffer;
        else
            return A.material < B.material;
    });

    m_renderer->RenderObjects(objects, order);
}

void RenderSystem::renderGui(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("SceneData");
    auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
    ImGui::InputFloat4("Ambient color", glm::value_ptr(sceneData->ambientColor));
    ImGui::End();

    ImGui::Begin("Stats");
    auto stats = m_renderer->GetRenderingStats();
    ImGui::Text(std::format("Frames per second: {}", ecs.GetSingletonComponent<FramesPerSecond>()->value).c_str());
    ImGui::Text(std::format("Draw calls count: {}", stats.drawcallCount).c_str());
    ImGui::Text(std::format("Triangle count: {}", stats.triangleCount).c_str());
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

    ImGui::Begin("Controller");
    ecs.Each<Controller>([](Hori::Entity, Controller& controller) {
        ImGui::InputFloat("Speed", &controller.speed);
        ImGui::InputFloat("Sensitivity", &controller.sensitivity);
    });
    ImGui::End();

    ImGui::Begin("Object info");
    std::vector<Hori::Entity> outOfDateTags;
    ecs.Each<RayTagged, Translation, Rotation, Scale, Property<Translation>, Property<Rotation>, Property<Scale>>
    ([&ecs, &outOfDateTags](Hori::Entity e, RayTagged, Translation& translation, Rotation& rotation, Scale& scale, Property<Translation>& pTranslation, Property<Rotation>& pRotation, Property<Scale>& pScale) {
        if (ecs.GetComponentArray<RayTagged>().Size() - outOfDateTags.size() > 1)
        {
            outOfDateTags.push_back(e);
            return;
        }

        ImGui::InputFloat3(pTranslation.label.c_str(), glm::value_ptr(translation.value));
        ImGui::InputFloat("pitch", &rotation.pitch);
        ImGui::InputFloat("yaw", &rotation.yaw);
        ImGui::InputFloat("roll", &rotation.roll);
        ImGui::InputFloat3(pScale.label.c_str(), glm::value_ptr(scale.value));
    });

    for (auto& e : outOfDateTags)
    {
        ecs.RemoveComponents<RayTagged>(e);
    }
    ImGui::End();

    ImGui::Render();
    m_renderer->RenderImGui();
}

void RenderSystem::renderColliders()
{
    /*
    auto& ecs = Ecs::GetInstance();
    std::vector<WireframeObject> objects;
    ecs.Each<Drawable, LocalToWorld, ColliderEntity>([&](Hori::Entity e, Drawable& drawable, LocalToWorld& localToWorld, ColliderEntity&) {
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
    */
}

bool RenderSystem::isVisible(const RenderObject& obj, const glm::mat4& viewproj)
{
    std::array<glm::vec3, 8> corners {
        glm::vec3 { 1, 1, 1 },
        glm::vec3 { 1, 1, -1 },
        glm::vec3 { 1, -1, 1 },
        glm::vec3 { 1, -1, -1 },
        glm::vec3 { -1, 1, 1 },
        glm::vec3 { -1, 1, -1 },
        glm::vec3 { -1, -1, 1 },
        glm::vec3 { -1, -1, -1 },
    };

    glm::mat4 matrix = viewproj * obj.transform;

    glm::vec3 min = { 1.5, 1.5, 1.5 };
    glm::vec3 max = { -1.5, -1.5, -1.5 };

    for (int c = 0; c < 8; c++) {
        // project each corner into clip space
        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

        // perspective correction
        v.x = v.x / v.w;
        v.y = v.y / v.w;
        v.z = v.z / v.w;

        min = glm::min(glm::vec3 { v.x, v.y, v.z }, min);
        max = glm::max(glm::vec3 { v.x, v.y, v.z }, max);
    }

    // check the clip space box is within the view
    if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
        return false;
    } else {
        return true;
    }
}
