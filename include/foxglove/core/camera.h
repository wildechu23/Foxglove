#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

class InputManager;

class Camera {
public:
	glm::vec3 velocity {0.f, 0.f, 0.f};
	glm::vec3 position {0.f, 0.f, 0.f};
	float pitch {0.f};
	float yaw {0.f};

	glm::mat4 get_view_matrix();
	glm::mat4 get_rotation_matrix();
    glm::mat4 get_projection_matrix();

	void update(const InputManager& input, float delta_time);
};
