#pragma once

#include <glm/glm.hpp>
#include <World.h>

#include "../Components/Components.h"

class CameraSystem : public Hori::System
{
public:
    CameraSystem();

    void Update(float dt) override;

private:
    static glm::mat4 perspectiveProjection(Camera& camera);
    static glm::mat4 orthographicProjection(Camera& camera);
    static glm::mat4 view(const glm::mat4& model);
};
