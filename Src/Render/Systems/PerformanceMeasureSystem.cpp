#include "../../../Include/YakiEngine/Render/Systems/PerformanceMeasureSystem.h"

#include "Ecs.h"
#include "Components/FramesPerSecond.h"

PerformanceMeasureSystem::PerformanceMeasureSystem()
{

}

void PerformanceMeasureSystem::Update(float dt)
{
    measureFps(dt);
}

void PerformanceMeasureSystem::measureFps(float dt)
{
    auto& ecs = Ecs::GetInstance();

    auto fps = ecs.GetSingletonComponent<FramesPerSecond>();
    fps->timer += dt;
    fps->frameCount++;

    if (fps->timer >= 1)
    {
        fps->timer -= 1;
        fps->value = fps->frameCount;
        fps->frameCount = 0;
    }
}
