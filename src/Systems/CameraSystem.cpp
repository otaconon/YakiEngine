#include "CameraSystem.h"

#include "../Ecs.h"

CameraSystem::CameraSystem() = default;

void CameraSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.Each<Camera, LocalToParent>([](Hori::Entity, Camera& camera, LocalToParent& localToParent) {
        camera.view = view(localToParent.value);
        if (camera.isPerspective)
            camera.projection = perspectiveProjection(camera);
        else
            camera.projection = orthographicProjection(camera);
        camera.viewProjection = camera.projection * camera.view;
    });
}

glm::mat4 CameraSystem::perspectiveProjection(Camera& camera)
{
    const float sx = 1.0f / std::tan(camera.fovx * 0.5f);
    const float sy = sx * (static_cast<float>(camera.aspectRatio.x) / static_cast<float>(camera.aspectRatio.y));

    glm::mat4 P(0.0f);
    P[0][0] = sx;
    P[1][1] = -sy;
    P[2][2] = -camera.far / (camera.far - camera.near);
    P[2][3] = -1.0f;
    P[3][2] = -(camera.far * camera.near) / (camera.far - camera.near);

    return P;
}

glm::mat4 CameraSystem::orthographicProjection(Camera& camera)
{
    const float halfWidth = camera.orthoWidth / 2;
    const float halfHeight = camera.orthoHeight / 2;
    const float left = -halfWidth, bottom = halfHeight;
    const float right = halfWidth, top = -halfHeight;

    glm::mat4 projection(0.f);
    projection[0][0] = 2.f/(right - left);
    projection[1][1] = 2.f/(top - bottom);
    projection[2][2] = -2.f/(camera.far - camera.near);
    projection[3] = {-(right+left)/(right-left), -(top+bottom)/(top-bottom), -(camera.far + camera.near)/(camera.far - camera.near), 1};

    return projection;
}

glm::mat4 CameraSystem::view(const glm::mat4& model)
{
    return glm::inverse(model);
}
