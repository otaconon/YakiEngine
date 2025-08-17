#pragma once

#include <HECS/World.h>

class GuiSystem : public Hori::System
{
public:
    GuiSystem();
    void Update(float dt) override;

private:

};