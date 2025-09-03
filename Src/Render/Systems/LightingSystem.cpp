#include "../../../Include/YakiEngine/Render/Systems/LightingSystem.h"

#include "Ecs.h"
#include "Components/DirectionalLight.h"
#include "Vulkan/VkTypes.h"
#include "Components/Translation.h"


LightingSystem::LightingSystem(Renderer* renderer)
    : m_renderer(renderer)
{}

void LightingSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    // TODO: This shouldn't be done every frame, its inefficient
    auto& lightData = m_renderer->GetGpuLightData();
    auto& directionalLights = lightData.directionalLights;
    auto& pointLights = lightData.pointLights;
    assert(ecs.GetComponentArray<DirectionalLight>().Size() <= directionalLights.size());
    size_t idx = 0;
    ecs.Each<DirectionalLight>([&directionalLights, &idx](Hori::Entity, DirectionalLight& dirLight) {
        directionalLights[idx++] = dirLight;
    });
    idx = 0;
    ecs.Each<PointLight, Translation>([&pointLights, &idx](Hori::Entity, PointLight& pointLight, Translation& pos) {
        pointLights[idx].color = pointLight.color;
        pointLights[idx].position = glm::vec4(pos.value, 1.f);
        idx++;
    });
}
