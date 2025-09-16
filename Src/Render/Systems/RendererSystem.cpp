#include "../../../Include/YakiEngine/Render/Systems/RendererSystem.h"
#include "Components/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <numeric>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Components/RayTagged.h"
#include "Components/DirectionalLight.h"
#include "Gui/ItemList.h"
#include "Components/CoreComponents.h"
#include "Components/RenderComponents.h"


RenderSystem::RenderSystem(Renderer *renderer)
  : m_renderer(renderer) {
}

void RenderSystem::Update(float dt) {
  auto &ecs = Ecs::GetInstance();
  auto &sceneData = m_renderer->GetGpuSceneData();
  Camera camera;
  ecs.Each<Camera, Translation>([&camera, &sceneData](Hori::Entity, Camera &cam, Translation &pos) {
    camera = cam;
    sceneData.eyePos = glm::vec4(pos.value, 1.0f);
  });
  sceneData.proj = camera.projection;
  sceneData.view = camera.view;
  sceneData.viewproj = camera.viewProjection;

  m_renderer->BeginRendering();
  m_renderer->Begin3DRendering();
  renderDrawablesIndirect(camera.viewProjection);
  m_renderer->End3DRendering();
  renderGui(dt);
  m_renderer->EndRendering();

  Hori::Entity selectedEntity{m_renderer->GetHoveredEntityId()};
  if (selectedEntity.Valid())
    ecs.AddComponents(selectedEntity, Hovered{});
}

void RenderSystem::renderDrawables(const glm::mat4 &viewproj) {
  auto &ecs = Ecs::GetInstance();

  size_t drawableCount = ecs.GetComponentArray<Drawable>().Size();
  std::vector<RenderObject> objects;
  objects.reserve(drawableCount);

  ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity e, Drawable &drawable, LocalToWorld &localToWorld) {
    for (auto &[startIndex, count, bounds, material] : drawable.mesh->surfaces) {
      RenderObject obj{
          .objectId = e.id,
          .indexCount = count,
          .firstIndex = startIndex,
          .indexBuffer = drawable.mesh->meshBuffers->indexBuffer.buffer,
          .mesh = drawable.mesh.get(),
          .material = material.get(),
          .bounds = bounds,
          .transform = localToWorld.value,
          .vertexBufferAddress = drawable.mesh->meshBuffers->vertexBufferAddress
      };

      //if (isVisible(obj, viewproj))
      objects.push_back(obj);
    }
  });

  std::vector<size_t> order(objects.size());
  std::ranges::iota(order, 0);

  std::ranges::sort(order, [&objects](const auto &iA, const auto &iB) {
    auto &A = objects[iA];
    auto &B = objects[iB];
    if (A.material == B.material)
      return A.indexBuffer < B.indexBuffer;
    return A.material < B.material;
  });

  m_renderer->RenderObjects(objects, order);
}

void RenderSystem::renderDrawablesIndirect(const glm::mat4 &viewProj) {
  auto &ecs = Ecs::GetInstance();

  size_t drawableCount = ecs.GetComponentArray<Drawable>().Size();
  RenderIndirectObjects objects;

  ecs.Each<Drawable, LocalToWorld>([&](Hori::Entity e, Drawable &drawable, LocalToWorld &localToWorld) {
    for (auto &[startIndex, count, bounds, material] : drawable.mesh->surfaces) {
      objects.objectIds.push_back(e.id);
      objects.transforms.push_back(localToWorld.value);
      objects.mesh = drawable.mesh.get();
      objects.material = material.get();
    }
  });

  m_renderer->UpdateGlobalDescriptor(objects.objectIds, objects.transforms);
  m_renderer->RenderObjectsIndirect(objects);
}

void RenderSystem::renderGui(float dt) {
  auto &ecs = Ecs::GetInstance();

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("object data")) {
      if (ImGui::MenuItem("point lights")) {
        m_showElements.flip(static_cast<size_t>(ShowImGui::PointLights));
      }
      if (ImGui::MenuItem("directional lights")) {
        m_showElements.flip(static_cast<size_t>(ShowImGui::DirectionalLights));
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  ImGui::Begin("SceneData");
  auto &sceneData = m_renderer->GetGpuSceneData();
  ImGui::InputFloat4("Ambient color", glm::value_ptr(sceneData.ambientColor));
  ImGui::End();

  ImGui::Begin("Stats");
  auto stats = m_renderer->GetRenderingStats();
  ImGui::Text("Frames per second: %d", static_cast<int>(ecs.GetSingletonComponent<FramesPerSecond>()->value));
  ImGui::Text("Draw calls count: %d", stats.drawcallCount);
  ImGui::Text("Triangle count: %d", stats.triangleCount);
  ImGui::End();

  if (m_showElements.test(static_cast<size_t>(ShowImGui::PointLights))) {
    ItemList<PointLight> pointLightList([](PointLight &pointLight) {
      ImGui::InputFloat4("Color", glm::value_ptr(pointLight.color));
    });
    pointLightList.Begin("Point lights", {"XXX"});
    uint32_t idx = 0;
    ecs.Each<PointLight>([&pointLightList, &idx](Hori::Entity, PointLight &light) {
      pointLightList.AddItem(light, std::format("point light {}", idx));
      idx++;
    });

    pointLightList.End();
  }

  if (m_showElements.test(static_cast<size_t>(ShowImGui::DirectionalLights))) {
    ItemList<DirectionalLight> pointLightList([](DirectionalLight &dirLight) {
      ImGui::InputFloat4("Color", glm::value_ptr(dirLight.color));
      ImGui::InputFloat4("Direction", glm::value_ptr(dirLight.direction));
    });
    pointLightList.Begin("Directional lights", {"XXX"});
    uint32_t idx = 0;
    ecs.Each<DirectionalLight>([&pointLightList, &idx](Hori::Entity, DirectionalLight &light) {
      pointLightList.AddItem(light, std::format("directional light {}", idx));
      idx++;
    });

    pointLightList.End();
  }

  ImGui::Begin("Controller");
  ecs.Each<Controller>([](Hori::Entity, Controller &controller) {
    ImGui::InputFloat("Speed", &controller.speed);
    ImGui::InputFloat("Sensitivity", &controller.sensitivity);
  });
  ImGui::End();

  ImGui::Begin("Object info");
  std::vector<Hori::Entity> outOfDateTags;
  ecs.Each<RayTagged, Translation, Rotation, Scale>
      ([&ecs, &outOfDateTags](Hori::Entity e, RayTagged, Translation &translation, Rotation &rotation, Scale &scale) {
        if (ecs.GetComponentArray<RayTagged>().Size() - outOfDateTags.size() > 1) {
          outOfDateTags.push_back(e);
          return;
        }

        ImGui::InputFloat3("translation", glm::value_ptr(translation.value));
        ImGui::InputFloat("pitch", &rotation.pitch);
        ImGui::SameLine();
        ImGui::InputFloat("yaw", &rotation.yaw);
        ImGui::SameLine();
        ImGui::InputFloat("roll", &rotation.roll);
        ImGui::InputFloat3("scale", glm::value_ptr(scale.value));
      });
  for (auto &e : outOfDateTags) {
    ecs.RemoveComponents<RayTagged>(e);
  }
  ImGui::End();

  ImGui::Render();
  m_renderer->RenderImGui();
}

bool RenderSystem::isVisible(const RenderObject &obj, const glm::mat4 &viewproj) {
  std::array<glm::vec3, 8> corners{
      glm::vec3{1, 1, 1},
      glm::vec3{1, 1, -1},
      glm::vec3{1, -1, 1},
      glm::vec3{1, -1, -1},
      glm::vec3{-1, 1, 1},
      glm::vec3{-1, 1, -1},
      glm::vec3{-1, -1, 1},
      glm::vec3{-1, -1, -1},
  };

  glm::mat4 matrix = viewproj * obj.transform;

  glm::vec3 min = {1.5, 1.5, 1.5};
  glm::vec3 max = {-1.5, -1.5, -1.5};

  for (int c = 0; c < 8; c++) {
    glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

    // perspective correction
    v.x = v.x / v.w;
    v.y = v.y / v.w;
    v.z = v.z / v.w;

    // Add or subtract 0.1f because of imprecision
    min = glm::min(glm::vec3{v.x, v.y, v.z}, min) - 0.1f;
    max = glm::max(glm::vec3{v.x, v.y, v.z}, max) + 0.1f;
  }

  // check the clip space box is within the view
  if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
    return false;
  }
  return true;
}