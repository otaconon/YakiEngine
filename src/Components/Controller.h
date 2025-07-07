#pragma once

#include <glm/glm.hpp>

enum class MouseMode
{
	EDITOR, GAME
};

struct Controller
{
	glm::vec3 direction{};
	float speed = 10.f;
	float mouseX = 0.f, mouseY = 0.f;
	float dx = 0.f, dy = 0.f;
	MouseMode mouseMode = MouseMode::EDITOR;
};
