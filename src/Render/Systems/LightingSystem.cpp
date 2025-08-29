#include "../../../include/YakiEngine/Render/Systems/LightingSystem.h"

#include "../../../include/YakiEngine/Core/Ecs.h"
#include "../../../include/YakiEngine/Render/Components/DirectionalLight.h"
#include "../Vulkan/VkTypes.h"
#include "../../../include/YakiEngine/Core/Components/Translation.h"


LightingSystem::LightingSystem() = default;

void LightingSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    // TODO: This shouldn't be done every frame, its inefficient
    auto lightData = ecs.GetSingletonComponent<GPULightData>();
    auto& directionalLights = lightData->directionalLights;
    auto& pointLights = lightData->pointLights;
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
