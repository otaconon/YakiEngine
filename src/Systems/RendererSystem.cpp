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
    ecs.Each<Camera>([&camera](Hori::Entity e, const Camera& cam) {
        camera = cam;
    });

    auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
    sceneData->proj = camera.projection;
    sceneData->view = camera.view;
    sceneData-> viewproj = camera.viewProjection;

    // TODO: Fix this not very fast
    std::vector<RenderObject> objects;
    ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity, Drawable& drawable, LocalToWorld& localToWorld) {
        for (auto& s : drawable.mesh->surfaces)
        {
            RenderObject def;
            def.indexCount = s.count;
            def.firstIndex = s.startIndex;
            def.indexBuffer = drawable.mesh->meshBuffers->indexBuffer.buffer;
            def.material = s.material.get();
            def.transform = localToWorld.value;
            def.vertexBufferAddress = drawable.mesh->meshBuffers->vertexBufferAddress;

            objects.push_back(def);
        }
    });

    m_renderer->DrawFrame(objects);
}
