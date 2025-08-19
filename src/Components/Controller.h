#pragma once

#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

enum class MouseMode
{
	EDITOR, EDITOR_CAMERA, GAME
};

struct Controller
{
	glm::vec3 direction{};
	glm::vec3 targetRotation{};
	float speed = 10.f;
	float mouseX = 0.f, mouseY = 0.f;
	float dx = 0.f, dy = 0.f;
	float smoothedDx = 0.f, smoothedDy = 0.f;
	float lockX = 0.f, lockY = 0.f;
	float sensitivity = 0.7f;
	bool mouseButtonLeftPressed = false;
	bool mouseButtonRightPressed = false;
	MouseMode mouseMode = MouseMode::EDITOR;
};
