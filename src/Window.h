#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <optional>
#include <glm/glm.hpp>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "utils.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/VulkanContext.h"

/*
const std::vector<Vertex> vertices = {
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 1
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // 2
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}}, // 3
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 4
    {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // 5
    {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}}, // 6
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}} // 7
};
const std::vector<uint16_t> indices = {
     0,  2,  1,
     0,  3,  2,
     4,  5,  6,
     4,  6,  7,
     2,  3,  7,
     2,  7,  6,
     1,  4,  0,
     1,  5,  4,
     0,  7,  3,
     0,  4,  7,
     1,  2,  6,
     1,  6,  5
};
*/

class Window {
public:
	Window();
	~Window();

	[[nodiscard]] SDL_Window* window();

private:
    SDL_Window* m_window;

	static bool resizingEventWatcher(void* data, SDL_Event* event);
};