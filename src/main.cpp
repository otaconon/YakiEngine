#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <iostream>

#include "Ecs.h"
#include "Vulkan/Renderer.h"
#include "Window.h"
#include "Camera/Camera.h"
#include "Systems/RendererSystem.h"
#include "Components/Controller.h"
#include "Systems/ControllerSystem.h"
#include "Systems/MovementSystem.h"
#include "InputData.h"
#include "Raycast.h"
#include "Components/Collider.h"
#include "Systems/PhysicsSystem.h"
#include "Components/gui.h"
#include "Systems/GuiSystem.h"

std::vector<Vertex> vertices = {
/* +Z (front)  */ {{-0.5f,-0.5f, 0.5f},{1,0,0},{0,0}}, // 0
                   {{ 0.5f,-0.5f, 0.5f},{0,1,0},{1,0}}, // 1
                   {{ 0.5f, 0.5f, 0.5f},{0,0,1},{1,1}}, // 2
                   {{-0.5f, 0.5f, 0.5f},{1,1,1},{0,1}}, // 3
/* –Z (back)   */ {{ 0.5f,-0.5f,-0.5f},{1,0,0},{0,0}}, // 4
                   {{-0.5f,-0.5f,-0.5f},{0,1,0},{1,0}}, // 5
                   {{-0.5f, 0.5f,-0.5f},{0,0,1},{1,1}}, // 6
                   {{ 0.5f, 0.5f,-0.5f},{1,1,1},{0,1}}, // 7
/* –X (left)   */ {{-0.5f,-0.5f,-0.5f},{1,0,0},{0,0}}, // 8
                   {{-0.5f,-0.5f, 0.5f},{0,1,0},{1,0}}, // 9
                   {{-0.5f, 0.5f, 0.5f},{0,0,1},{1,1}}, // 10
                   {{-0.5f, 0.5f,-0.5f},{1,1,1},{0,1}}, // 11
/* +X (right)  */ {{ 0.5f,-0.5f, 0.5f},{1,0,0},{0,0}}, // 12
                   {{ 0.5f,-0.5f,-0.5f},{0,1,0},{1,0}}, // 13
                   {{ 0.5f, 0.5f,-0.5f},{0,0,1},{1,1}}, // 14
                   {{ 0.5f, 0.5f, 0.5f},{1,1,1},{0,1}}, // 15
/* +Y (top)    */ {{-0.5f, 0.5f, 0.5f},{1,0,0},{0,0}}, // 16
                   {{ 0.5f, 0.5f, 0.5f},{0,1,0},{1,0}}, // 17
                   {{ 0.5f, 0.5f,-0.5f},{0,0,1},{1,1}}, // 18
                   {{-0.5f, 0.5f,-0.5f},{1,1,1},{0,1}}, // 19
/* –Y (bottom) */ {{-0.5f,-0.5f,-0.5f},{1,0,0},{0,0}}, // 20
                   {{ 0.5f,-0.5f,-0.5f},{0,1,0},{1,0}}, // 21
                   {{ 0.5f,-0.5f, 0.5f},{0,0,1},{1,1}}, // 22
                   {{-0.5f,-0.5f, 0.5f},{1,1,1},{0,1}}  // 23
};

std::vector<uint32_t> indices = {
    0,1,2,  2,3,0,       // +Z
    4,5,6,  6,7,4,       // –Z
    8,9,10, 10,11,8,     // –X
    12,13,14, 14,15,12,  // +X
    16,17,18, 18,19,16,  // +Y
    20,21,22, 22,23,20   // –Y
};

Drawable create_drawable(const std::shared_ptr<GPUMeshBuffers>& mesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
	Drawable drawable{mesh, vertices, indices, {glm::mat4{1.f}, glm::mat4{}, glm::mat4{}}};
	return drawable;
}

int main() {
    auto& ecs = Ecs::GetInstance();

	Window mainWindow;
	auto vk = std::make_shared<VulkanContext>(mainWindow.window());

	Renderer renderer(mainWindow.window(), vk);
	ecs.AddSystem<RenderSystem>(renderer);
	ecs.AddSystem<ControllerSystem>(ControllerSystem());
	ecs.AddSystem<MovementSystem>(MovementSystem());
	ecs.AddSystem<PhysicsSystem>(PhysicsSystem());

	ecs.AddSingletonComponent(InputEvents{});

	Drawable drawable = create_drawable(renderer.UploadMesh(std::span(indices), std::span(vertices)), vertices, indices);
	Hori::Entity poo = ecs.CreateEntity();
	ecs.AddComponents(poo, std::move(drawable), Transform{{0.0f, 0.0f, 0.0f}, {0.f, 0.f, 0.f}, {5.f, 5.f, 5.f}}, BoxCollider{{0.5f, 0.5f, 0.5f}, true});

	Hori::Entity camera = ecs.CreateEntity();
	Transform camTrans{{0.f, -10.f, -10.f}, glm::vec3{0.f}, glm::vec3{1.f}};
	camTrans.LookAt({0.f, 0.f, 0.f});
	ecs.AddComponents(camera, Camera{}, Controller{}, RayData{});
	ecs.AddComponents(camera, std::move(camTrans));

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
		vkDeviceWaitIdle(vk->GetDevice());
		ecs.GetSingletonComponent<InputEvents>()->Clear();
		prevTime = currentTime;
		std::cout.flush();
	}

	ecs.Destroy();
}