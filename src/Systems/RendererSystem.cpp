#include "RendererSystem.h"
#include "../Components/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

RenderSystem::RenderSystem(Renderer& renderer)
    : m_renderer(&renderer)
{}

void RenderSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    Camera camera;
    glm::mat4 cameraTransform;
    ecs.Each<Camera, LocalToParent>([&camera, &cameraTransform](Hori::Entity e, const Camera& cam, LocalToParent transform) {
        camera = cam;
        cameraTransform = transform.value;
    });

    std::vector<Drawable> drawables;
    ecs.Each<Drawable, LocalToParent>([&](Hori::Entity, Drawable& drawable, LocalToParent& transform) {
        drawable.ubo.model = transform.value;
        drawable.ubo.view = camera.view;
        drawable.ubo.proj = camera.projection;
        drawables.push_back(drawable);
    });

    m_renderer->DrawFrame(drawables);
}
