#pragma once

#include <glm/glm.hpp>

enum class MouseMode
{
	EDITOR, EDITOR_CAMERA, GAME
};

struct Controller
{
	glm::vec3 direction{};
	float speed = 10.f;
	float mouseX = 0.f, mouseY = 0.f;
	float dx = 0.f, dy = 0.f;
	float lockX = 0.f, lockY = 0.f;
	bool mouseButtonLeftPressed = false;
	bool mouseButtonRightPressed = false;
	MouseMode mouseMode = MouseMode::EDITOR;
};
