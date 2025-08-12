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
#include "Systems/GuiSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/CameraSystem.h"
#include "Components/Components.h"
#include "Components/DirectionalLight.h"
#include "Vulkan/Gltf/GltfUtils.h"
#include "PropertyRegistry.h"
#include "Systems/LightingSystem.h"

Drawable create_drawable(std::shared_ptr<Mesh> mesh)
{
	Drawable drawable{mesh, {}, {}, {glm::mat4{1.f}, glm::mat4{}, glm::mat4{}}};
	return drawable;
}

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
		ecs.AddComponents(e, create_drawable(mesh), Translation{{3.f * idx, 1.f, 1.f}}, Rotation{}, Scale{{1.f, 1.f, 1.f}}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, BoxCollider{{0.5f, 0.5f, 0.5f}, true});
		register_property<Translation>(e, "Translation");
		register_property<Rotation>(e, "Rotation");
		register_property<Scale>(e, "Scale");
	}

	// Create camera entity
	Hori::Entity camera = ecs.CreateEntity();
	ecs.AddComponents(camera, Camera{}, Controller{}, RayData{});
	ecs.AddComponents(camera, Translation{{0, -10.f, -10.f}}, Rotation{}, Scale{}, LocalToWorld{}, LocalToParent{}, ParentToLocal{});

	// Create light entity
	Hori::Entity light = ecs.CreateEntity();
	ecs.AddComponents(light, DirectionalLight({0.5f, 0.3f, 0.2f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}));
	ecs.AddComponents(light, PointLight({0.5f, 0.3f, 0.2f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}));
	ecs.AddComponents(light, create_drawable(allMeshes[1]), Translation{{10.f, 10.f, 1.f}}, Rotation{}, Scale{{1.f, 1.f, 1.f}}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, BoxCollider{{0.5f, 0.5f, 0.5f}, true});
	register_property<Translation>(light, "Translation");
	register_property<Rotation>(light, "Rotation");
	register_property<Scale>(light, "Scale");

	// Initialize scene
	auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
	sceneData->ambientColor = glm::vec4(.1f, .1f, .1f, 1.f);

	auto prevTime = std::chrono::high_resolution_clock::now();
	bool running = true;
	SDL_Event event;
	while (running)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();

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