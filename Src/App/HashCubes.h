#pragma once

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

class HashCubes
{
public:
	HashCubes();
	~HashCubes();

	void Run();

private:
	Window m_window{};
	VulkanContext m_ctx;
	Renderer m_renderer;

	MaterialBuilder m_materialBuilder;
	DeletionQueue m_deletionQueue;

	std::shared_ptr<GltfObject> m_allMeshes;
	std::shared_ptr<Mesh> m_cubeMesh;

private:
	void initEcs();
};