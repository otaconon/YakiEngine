#pragma once

#include <HECS/World.h>

class PerformanceMeasureSystem : public Hori::System
{
public:
    PerformanceMeasureSystem();

    void Update(float dt) override;

private:
    void measureFps(float dt);
};