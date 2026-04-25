#include "foxglove/core/camera.h"
#include "foxglove/core/input_manager.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

glm::mat4 Camera::get_view_matrix() {
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
	glm::mat4 cameraRotation = get_rotation_matrix();
	return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::get_rotation_matrix() {
	glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3(1.f, 0.f, 0.f));
	glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3(0.f, -1.f, 0.f));
	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

glm::mat4 Camera::get_projection_matrix() {
    glm::mat4 proj = glm::perspective(
        glm::radians(70.0f),
        1280.f / 720.f,
        1000.f,
        0.1f
    );
    proj[1][1] *= -1;  // Vulkan Y-flip
    return proj;

}

void Camera::update(const InputManager& input, float delta_time) {
    if (input.is_key_held(GLFW_KEY_W)) { velocity.z = -1; }
    else if (input.is_key_held(GLFW_KEY_S)) { velocity.z = 1; }
    else { velocity.z = 0; }
    
    if (input.is_key_held(GLFW_KEY_A)) { velocity.x = -1; }
    else if (input.is_key_held(GLFW_KEY_D)) { velocity.x = 1; }
    else { velocity.x = 0; }
    
    if (input.is_key_held(GLFW_KEY_LEFT_SHIFT)) { velocity.y = -1; }
    else if (input.is_key_held(GLFW_KEY_SPACE)) { velocity.y = 1; }
    else { velocity.y = 0; }

    // TODO: FIX MOUSE MOVEMENT


	glm::mat4 camera_rotation = get_rotation_matrix();
	position += glm::vec3(camera_rotation * 
            glm::vec4(velocity * delta_time, 0.f));

    glm::vec2 mouse_delta = input.get_mouse_delta();
    yaw += mouse_delta.x / 300.f;
    pitch += -mouse_delta.y / 300.f;

    //std::cout << position.x << ", " << position.z << std::endl;
}
