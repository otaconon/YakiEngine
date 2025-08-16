#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <tracy/Tracy.hpp>

#include "Ecs.h"
#include "Vulkan/Renderer.h"
#include "Window.h"
#include "Systems/RendererSystem.h"
#include "Systems/MovementSystem.h"
#include "Raycast.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/CameraSystem.h"
#include "Components/Components.h"
#include "Systems/InputSystem.h"
#include "Systems/LightingSystem.h"
#include "Assets/AssetMngr.h"
#include "Assets/GltfUtils.h"

constexpr uint32_t numDirectionalLights = 0;
constexpr uint32_t numPointLights = 2;

int main() {
    auto& ecs = Ecs::GetInstance();

	Window mainWindow;
	VulkanContext ctx{mainWindow.window()};
	Renderer renderer(mainWindow.window(), &ctx);
	DeletionQueue deletionQueue;

	AssetMngr::Initialize(&ctx);

	ecs.AddSystem<InputSystem>(InputSystem(mainWindow.window()));
	ecs.AddSystem<CameraSystem>(CameraSystem());
	ecs.AddSystem<MovementSystem>(MovementSystem());
	ecs.AddSystem<TransformSystem>(TransformSystem());
	ecs.AddSystem<LightingSystem>(LightingSystem());
	ecs.AddSystem<RenderSystem>(renderer);
	ecs.AddSystem<PhysicsSystem>(PhysicsSystem());

	ecs.AddSingletonComponent(GPUSceneData{});
	ecs.AddSingletonComponent(GPULightData{});
	ecs.AddSingletonComponent(MouseMode{});

	DefaultData defaultData = create_default_data(&ctx, deletionQueue);
	renderer.InitDefaultData(defaultData);

	// Create object entities
	auto allMeshes = AssetMngr::LoadMeshes("../assets/meshes/basicmesh.glb");
	for (auto [idx, meshHandle] : std::views::enumerate(allMeshes))
	{
		Hori::Entity e = ecs.CreateEntity();
		register_object(e, AssetMngr::GetAsset<Mesh>(meshHandle), Translation{{3.f * idx, 0.f, 0.f}});
	}

	// Load scene
	auto scene = GltfUtils::load_gltf_object(&ctx, "../assets/scenes/Bakalarska.glb", renderer, defaultData);
	for (auto& e: scene.value()->nodes | std::views::values)
	{
		if (!e.Valid())
			continue;
		if (ecs.HasComponents<Mesh>(e))
		{
			auto eMesh = ecs.GetComponent<Mesh>(e);
			ecs.AddComponents(e, Drawable{std::make_shared<Mesh>(*eMesh), {}, {}, {glm::mat4{1.f}, glm::mat4{}, glm::mat4{}}});
		}
	}

	// Create camera entity
	Hori::Entity camera = ecs.CreateEntity();
	ecs.AddComponents(camera, Camera{}, Controller{}, RayData{});
	ecs.AddComponents(camera, Translation{{0, -10.f, -10.f}}, Rotation{}, Scale{}, LocalToWorld{}, LocalToParent{}, ParentToLocal{});

	// Init lights data
	auto lightData = ecs.GetSingletonComponent<GPULightData>();
	lightData->numDirectionalLights = numDirectionalLights;
	lightData->numPointLights = numPointLights;

	// Create light entity
	std::array<Hori::Entity, numPointLights> lights;
	for (auto& e : lights)
	{
		e = ecs.CreateEntity();
		ecs.AddComponents(e, PointLight({0.5f, 0.3f, 0.2f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}));
		register_object(e, AssetMngr::GetAsset<Mesh>(allMeshes[1]), Translation{{10.f, 10.f, 1.f}});
	}

	// Initialize scene
	auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
	sceneData->ambientColor = glm::vec4(.1f, .1f, .1f, 1.f);

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