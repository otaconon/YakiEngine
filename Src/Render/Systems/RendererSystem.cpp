#include "Systems/RendererSystem.h"
#include "Components/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <numeric>

#include "Components/CoreComponents.h"
#include "Components/DirectionalLight.h"
#include "Components/DynamicObject.h"
#include "Components/RayTagged.h"
#include "Components/RenderComponents.h"
#include "Gui/ItemList.h"

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
  renderStaticObjects(camera.viewProjection);
  m_renderer->End3DRendering();
  renderGui(dt);
  m_renderer->EndRendering();

  Hori::Entity selectedEntity{m_renderer->GetHoveredEntityId()};
  if (selectedEntity.Valid())
    ecs.AddComponents(selectedEntity, Hovered{});
}

void RenderSystem::renderStaticObjects(const glm::mat4 &viewProj) {
  auto &ecs = Ecs::GetInstance();

  if (ecs.GetComponentArray<DirtyStaticObject>().Size() != 0) {
    RenderIndirectObjects objects;
    ecs.Each<DirtyStaticObject, StaticObject, LocalToWorld>([&](Hori::Entity e, DirtyStaticObject, StaticObject &drawable, LocalToWorld &localToWorld) {
      for (auto &[startIndex, vertexOffset, count, bounds, material] : drawable.mesh->surfaces) {
        objects.firstIndices.push_back(startIndex);
        objects.indexCounts.push_back(count);
        objects.vertexOffsets.push_back(vertexOffset);
        objects.objectIds.push_back(e.id);
        objects.transforms.push_back(localToWorld.value);
        objects.meshes.push_back(drawable.mesh.get());
        objects.materials.push_back(material.get());
      }
    });
    ecs.Each<DirtyStaticObject>([&](Hori::Entity e, DirtyStaticObject) {
      ecs.RemoveComponents<DirtyStaticObject>(e);
    });
    m_renderer->UpdateStaticObjects(objects);
    m_indirectBatches = packObjects(objects);
  }

  m_renderer->RenderStaticObjects(m_indirectBatches);
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
  ecs.Each<RayTagged, Translation, Rotation, Scale>([&ecs, &outOfDateTags](Hori::Entity e, RayTagged, Translation &translation, Rotation &rotation, Scale &scale) {
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

std::vector<IndirectBatch> RenderSystem::packObjects(RenderIndirectObjects &objects) {
  std::vector<uint32_t> order(objects.objectIds.size());
  std::iota(order.begin(), order.end(), 0);

  std::ranges::sort(order, {}, [&](uint32_t i) {
    return std::pair{objects.materials[i], objects.meshes[i]};
  });

  RenderIndirectObjects newObjects;
  const auto n = static_cast<size_t>(order.size());

  newObjects.firstIndices.resize(n);
  newObjects.indexCounts.resize(n);
  newObjects.vertexOffsets.resize(n);
  newObjects.objectIds.resize(n);
  newObjects.transforms.resize(n);
  newObjects.meshes.resize(n);
  newObjects.materials.resize(n);

  for (size_t pos = 0; pos < n; ++pos) {
    const auto idx = order[pos];
    newObjects.firstIndices[pos] = objects.firstIndices[idx];
    newObjects.indexCounts[pos] = objects.indexCounts[idx];
    newObjects.vertexOffsets[pos] = objects.vertexOffsets[idx];
    newObjects.objectIds[pos] = objects.objectIds[idx];
    newObjects.transforms[pos] = objects.transforms[idx];
    newObjects.meshes[pos] = objects.meshes[idx];
    newObjects.materials[pos] = objects.materials[idx];
  }

  std::vector<IndirectBatch> draws;
  draws.push_back({
      .indexCount = newObjects.indexCounts[0],
      .firstIndex = newObjects.firstIndices[0],
      .vertexOffset = newObjects.vertexOffsets[0],
      .firstInstance = 0,
      .instanceCount = 1,
      .mesh = newObjects.meshes[0],
      .material = newObjects.materials[0],
  });

  for (uint32_t i = 1; i < newObjects.objectIds.size(); i++) {
    if (newObjects.meshes[i - 1] != newObjects.meshes[i] || newObjects.materials[i - 1] != newObjects.materials[i]) {
      draws.push_back({
          .indexCount = 0,
          .firstIndex = newObjects.firstIndices[i],
          .vertexOffset = newObjects.vertexOffsets[i],
          .firstInstance = i,
          .instanceCount = 0,
          .mesh = newObjects.meshes[i],
          .material = newObjects.materials[i],
      });
    }
    draws.back().instanceCount++;
    draws.back().indexCount += newObjects.indexCounts[i];
  }

  return draws;
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