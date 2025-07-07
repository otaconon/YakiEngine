#include "PhysicsSystem.h"

#include <algorithm>
#include <print>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "../Components/Collider.h"
#include "../Ecs.h"
#include "../Raycast.h"
#include "../Transform.h"
#include "../InputData.h"
#include "../Camera/Camera.h"
#include "../Components/Controller.h"

PhysicsSystem::PhysicsSystem() = default;

void PhysicsSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    Camera camera;
    Transform cameraTransform;
    bool castRay = false;

    for (auto& button : ecs.GetSingletonComponent<InputEvents>()->mouseButton)
        if (button.button == SDL_BUTTON_LEFT)
            castRay = true;

    ecs.Each<Camera, Transform, Controller, RayData>([&](Hori::Entity, Camera& cam, Transform& transform, Controller& controller, RayData& ray) {
        camera = cam;
        cameraTransform = transform;

        if (castRay && controller.mouseMode == MouseMode::GAME)
        {
            ray.active = true;
            ray.origin = cameraTransform.GetPosition();
            ray.dir = cameraTransform.GetForward();
            ray.hit = RayHit{};
        }
        else if (castRay && controller.mouseMode == MouseMode::EDITOR)
        {
            glm::ivec2 aspectRatio = cam.GetAspectRatio();
            float ndcX =  (2.0f * controller.mouseX) / static_cast<float>(aspectRatio.x) - 1.f;
            float ndcY =  (2.0f * controller.mouseY) / static_cast<float>(aspectRatio.y) - 1.f;

            glm::vec4 clipNear = glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
            glm::vec4 clipFar  = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

            glm::mat4 invViewProj = glm::inverse(camera.GetPerspectiveProjection() * camera.GetView(cameraTransform.GetModel()));

            glm::vec4 pNear = invViewProj * clipNear;
            pNear /= pNear.w;

            glm::vec4 pFar  = invViewProj * clipFar;
            pFar  /= pFar.w;

            ray.active = true;
            ray.origin = cameraTransform.GetPosition();
            ray.dir = glm::normalize(glm::vec3(pFar) - ray.origin);
            ray.hit = RayHit{};
        }
    });

    ecs.Each<RayData>([&ecs](Hori::Entity, RayData& ray) {
        if (!ray.active)
            return;

        constexpr float epsilon = 1e-4f;
        ecs.Each<SphereCollider, Transform>([&ecs, &ray](Hori::Entity& e, SphereCollider& collider, Transform& transform) {
            glm::vec3 pos = transform.GetPosition();
            glm::vec3 oc = ray.origin - pos;
            float b = glm::dot(oc, ray.dir);
            float c = glm::length2(oc) - collider.radius*collider.radius;
            float disc = b*b - c;

            if (disc < 0.0f)
                return;

            float t = -b - std::sqrt(disc);
            if (t < epsilon)
                t = -b + std::sqrt(disc);
            if (t < epsilon)
                return;

            const float dist = glm::length(oc);
            ray.hit = ray.hit.dist < dist ? RayHit{e, pos, dist} : ray.hit;
        });

        ecs.Each<BoxCollider, Transform>([&ecs, &ray](Hori::Entity& e, BoxCollider& collider, Transform& transform) {
            float tNear = -1e5f, tFar = 1e5f;
            glm::vec3 pos = transform.GetPosition();
            glm::mat3 invTransform = glm::inverse(glm::mat3(transform.GetModel()));
            glm::vec3 localOrigin = invTransform * (ray.origin - pos);
            glm::vec3 localDir = invTransform * ray.dir;

            for (int i = 0; i < 3; i++) {
                if (glm::abs(localDir[i]) < epsilon)
                {
                    if (localOrigin[i] < -collider.dims[i] || localOrigin[i] > collider.dims[i])
                        return;
                }
                else
                {
                    float t1 = (-collider.dims[i] - localOrigin[i]) / localDir[i];
                    float t2 = (collider.dims[i] - localOrigin[i]) / localDir[i];

                    if (t1 > t2)
                        std::swap(t1, t2);

                    tNear = std::max(tNear, t1);
                    tFar = std::min(tFar, t2);

                    if (tNear > tFar)
                        return;
                }
            }

            if (tFar < 0)
                return;

            float hitT = std::max(tNear, 0.f);
            glm::vec3 hitPoint = ray.origin + hitT * ray.dir;
            if (hitT < ray.hit.dist)
                ray.hit = RayHit{ e, hitPoint, hitT };
        });

        if (ray.hit.e.Valid())
            std::print("Ray had hit an entity: {}\n", ray.hit.e.id);

        ray.hit.e.id = 0;
        ray.active = false;
    });
}
