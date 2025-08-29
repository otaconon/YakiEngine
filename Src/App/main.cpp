#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <tracy/Tracy.hpp>

#include "Ecs.h"
#include "Vulkan/Renderer.h"
#include "SDL/Window.h"
#include "Systems/RendererSystem.h"
#include "Systems/MovementSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/InputSystem.h"
#include "Systems/LightingSystem.h"
#include "Assets/AssetMngr.h"
#include "Assets/GltfUtils.h"
#include "Components/RenderComponents.h"
#include "Components/CoreComponents.h"
#include "HECS/Core/Entity.h"
#include "Systems/PerformanceMeasureSystem.h"
#include "Vulkan/VulkanContext.h"
#include "Assets/utils.h"
#include "SDL3/SDL_events.h"

constexpr uint32_t numDirectionalLights = 1;
constexpr uint32_t numPointLights = 1;

int main() {
    auto& ecs = Ecs::GetInstance();

	Window mainWindow;
	VulkanContext ctx{mainWindow.window()};
	Renderer renderer(mainWindow.window(), &ctx);
	DeletionQueue deletionQueue;
	
	MaterialBuilder materialBuilder(&ctx);
	materialBuilder.BuildPipelines(renderer.GetSwapchain(), renderer.GetSceneDataDescriptorLayout());
	AssetMngr::Initialize(&ctx, &materialBuilder);

	ecs.AddSystem<InputSystem>(InputSystem(mainWindow.window()));
	ecs.AddSystem<CameraSystem>(CameraSystem());
	ecs.AddSystem<MovementSystem>(MovementSystem());
	ecs.AddSystem<TransformSystem>(TransformSystem());
	ecs.AddSystem<LightingSystem>(&renderer);
	ecs.AddSystem<RenderSystem>(&renderer);
	ecs.AddSystem<PerformanceMeasureSystem>(PerformanceMeasureSystem());

	ecs.AddSingletonComponent(FramesPerSecond{});
	ecs.AddSingletonComponent(MouseMode{});

	// Create object entities
	auto allMeshes = AssetMngr::LoadGltf("Assets/meshes/basicmesh.glb");

	// Load scene
	auto scene = AssetMngr::LoadGltf("Assets/scenes/Sponza.glb");

	// Create camera entity
	Hori::Entity camera = ecs.CreateEntity();
	ecs.AddComponents(camera, Camera{}, Controller{});
	ecs.AddComponents(camera, Translation{{0, -10.f, -10.f}}, Rotation{}, Scale{}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, Parent{}, Children{});

	// Init lights data
	auto& lightData = renderer.GetGpuLightData();
	lightData.numDirectionalLights = numDirectionalLights;
	lightData.numPointLights = numPointLights;

	// Create point lights
	std::array<Hori::Entity, numPointLights> pointLights;
	for (auto& e : pointLights)
	{
		e = ecs.CreateEntity();
		ecs.AddComponents(e, PointLight{{0.5f, 0.5f, 0.5f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}});
		auto mesh = allMeshes->meshes.begin()->second;
		register_object(e, mesh, Translation{{3.f, 3.f, 0.f}});
	}

	// Create directional lights
	std::array<Hori::Entity, numDirectionalLights> directionalLights;
	for (auto& e : directionalLights)
	{
		e = ecs.CreateEntity();
		ecs.AddComponents(e, DirectionalLight{{0.5f, 0.5f, 0.5f, 1.f}, {0.5f, 0.5f, 0.0f, 1.f}});
		auto mesh = allMeshes->meshes.begin()->second;
		register_object(e, mesh, Translation{{3.f, 10.f, 0.f}});
	}

	/*
	auto boxMesh = std::next(allMeshes->meshes.begin(), 1)->second;
	int cnt = 0;
	ecs.Each<BoxCollider>([&ecs, &boxMesh, &cnt](Hori::Entity e, BoxCollider& boxCollider) {
		auto eCollider = ecs.CreateEntity();
		register_object(eCollider, boxMesh, {});
		ecs.AddComponents(eCollider, ColliderEntity{}, LocalToWorld{}, LocalToParent{}, Children{}, Parent{e});
		cnt++;
	});*/

	// Initialize scene
    auto& sceneData = renderer.GetGpuSceneData();
	sceneData.ambientColor = glm::vec4(.1f, .1f, .1f, 1.f);

	// Initialize input singleton components
	ecs.AddSingletonComponent(InputQueue<SDL_KeyboardEvent>());
	ecs.AddSingletonComponent(InputQueue<SDL_MouseButtonEvent>());
	ecs.AddSingletonComponent(InputQueue<SDL_MouseMotionEvent>());

	auto prevTime = std::chrono::high_resolution_clock::now();
	bool running = true;
	SDL_Event event;
	while (running)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float>(currentTime - prevTime).count();

		auto keyboardQueue = ecs.GetSingletonComponent<InputQueue<SDL_KeyboardEvent>>();
		auto mouseButtonQueue = ecs.GetSingletonComponent<InputQueue<SDL_MouseButtonEvent>>();
		auto mouseMotionQueue = ecs.GetSingletonComponent<InputQueue<SDL_MouseMotionEvent>>();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;
				case SDL_EVENT_KEY_DOWN:
					keyboardQueue->queue.push_back(event.key);
					break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					mouseButtonQueue->queue.push_back(event.button);
					break;
				case SDL_EVENT_MOUSE_BUTTON_UP:
					mouseButtonQueue->queue.push_back(event.button);
					break;
				case SDL_EVENT_MOUSE_MOTION:
					mouseMotionQueue->queue.push_back(event.motion);
					break;
				default:
					break;
			}

			ImGui_ImplSDL3_ProcessEvent(&event);
		}

		ecs.UpdateSystems(dt);
		renderer.WaitIdle();
		prevTime = currentTime;
		std::cout.flush();
		FrameMark;
	}

	deletionQueue.Flush();
	ecs.Destroy();
	AssetMngr::Shutdown();
}
