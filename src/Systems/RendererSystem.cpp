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

    // TODO: Fix this not very fast
    std::vector<Drawable> drawables;
    std::vector<RenderObject> objects;
    ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity, Drawable& drawable, LocalToWorld& localToWorld) {
        drawable.ubo.model = localToWorld.value;
        drawable.ubo.view = camera.view;
        drawable.ubo.proj = camera.projection;
        drawables.push_back(drawable);


    });

    m_renderer->DrawFrame(drawables);
}
