#include "LightingSystem.h"

#include "../Ecs.h"
#include "../Components/DirectionalLight.h"
#include "../Vulkan/VkTypes.h"

LightingSystem::LightingSystem()
{

}

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
    ecs.Each<PointLight>([&pointLights, &idx](Hori::Entity, PointLight& pointLight) {
        pointLights[idx++] = pointLight;
    });
}
