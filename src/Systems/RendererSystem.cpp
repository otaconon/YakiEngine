#include "RendererSystem.h"
#include "../Camera/Camera.h"

RenderSystem::RenderSystem(Renderer& renderer)
    : m_renderer(&renderer)
{}

void RenderSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    Camera camera;
    Transform cameraTransform;
    ecs.Each<Camera, Transform>([&camera, &cameraTransform](Hori::Entity e, const Camera& cam, const Transform& transform) {
        camera = cam;
        cameraTransform = transform;
    });

    std::vector<Drawable> drawables;
    ecs.Each<Drawable, Transform>([&](Hori::Entity, Drawable& drawable, Transform& transform) {
        transform.Rotate({1.f, 1.f, 0.f}, dt);

        drawable.ubo.model = transform.GetModel();
        drawable.ubo.view = camera.GetView(cameraTransform.GetModel());
        drawable.ubo.proj = camera.GetPerspectiveProjection();
        drawables.push_back(drawable);
    });

    m_renderer->DrawFrame(drawables);
}
