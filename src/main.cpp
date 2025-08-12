#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <iostream>

#include "Ecs.h"
#include "Vulkan/Renderer.h"
#include "Window.h"
#include "Systems/RendererSystem.h"
#include "Systems/ControllerSystem.h"
#include "Systems/MovementSystem.h"
#include "InputData.h"
#include "Raycast.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/CameraSystem.h"
#include "Components/Components.h"
#include "Vulkan/Gltf/GltfUtils.h"
#include "Systems/LightingSystem.h"

constexpr uint32_t numDirectionalLights = 0;
constexpr uint32_t numPointLights = 2;

int main() {
    auto& ecs = Ecs::GetInstance();

	Window mainWindow;
	VulkanContext ctx{mainWindow.window()};
	Renderer renderer(mainWindow.window(), &ctx);
	ecs.AddSystem<CameraSystem>(CameraSystem());
	ecs.AddSystem<ControllerSystem>(ControllerSystem());
	ecs.AddSystem<MovementSystem>(MovementSystem());
	ecs.AddSystem<TransformSystem>(TransformSystem());
	ecs.AddSystem<LightingSystem>(LightingSystem());
	ecs.AddSystem<RenderSystem>(renderer);
	ecs.AddSystem<PhysicsSystem>(PhysicsSystem());

	ecs.AddSingletonComponent(InputEvents{});
	ecs.AddSingletonComponent(GPUSceneData{});
	ecs.AddSingletonComponent(GPULightData{});

	// Create object entities
	auto allMeshes = GltfUtils::load_gltf_meshes(&ctx, "../assets/meshes/basicmesh.glb").value();
	for (auto [idx, mesh] : std::views::enumerate(allMeshes))
	{
		Hori::Entity e = ecs.CreateEntity();
		register_object(e, mesh, Translation{{3.f * idx, 0.f, 0.f}});
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
		register_object(e, allMeshes[1], Translation{{10.f, 10.f, 1.f}});
	}

	// Initialize scene
	auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
	sceneData->ambientColor = glm::vec4(.1f, .1f, .1f, 1.f);

	auto prevTime = std::chrono::high_resolution_clock::now();
	bool running = true;
	SDL_Event event;
	while (running)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float>(currentTime - prevTime).count();

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;
				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_ESCAPE)
					{
						SDL_SetWindowRelativeMouseMode(mainWindow.window(), false);
						SDL_CaptureMouse(false);
						ecs.GetComponent<Controller>(camera)->mouseMode = MouseMode::EDITOR;
					}
					else if (event.key.key == SDLK_SPACE)
					{
						SDL_SetWindowRelativeMouseMode(mainWindow.window(), true);
						SDL_CaptureMouse(true);
						ecs.GetComponent<Controller>(camera)->mouseMode = MouseMode::GAME;
					}
					else
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
						ecs.GetSingletonComponent<InputEvents>()->mouseButton.push_back(event.button);
						break;
				case SDL_EVENT_MOUSE_MOTION:
					ecs.GetSingletonComponent<InputEvents>()->mouseMotion.push_back(event.motion);
					break;
				default:
					break;
			}

			ImGui_ImplSDL3_ProcessEvent(&event);
		}

		ecs.UpdateSystems(dt);
		renderer.WaitIdle();
		ecs.GetSingletonComponent<InputEvents>()->Clear();
		prevTime = currentTime;
		std::cout.flush();
	}

	ecs.Destroy();
}