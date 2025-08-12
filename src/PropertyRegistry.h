#pragma once
#include <Entity.h>
#include <string>

#include "Ecs.h"
#include "Components/Property.h"

template<typename PropertyType>
void register_property(Hori::Entity e, const std::string& label)
{
    auto& ecs = Ecs::GetInstance();
    if (!ecs.HasComponents<PropertyType>(e))
        ecs.AddComponents(e, PropertyType{});

    ecs.AddComponents(e, Property<PropertyType>(label));
}