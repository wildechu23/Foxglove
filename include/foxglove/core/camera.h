#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

class InputManager;

class Camera {
public:
    Camera(float fov, float near, float far, float aspect_ratio)
        : m_fov(fov), m_near_plane(near), 
          m_far_plane(far), m_aspect_ratio(aspect_ratio) {}

    void set_aspect_ratio(float ratio) { m_aspect_ratio = ratio; } 
    void set_fov(float fov) { m_fov = fov; }
    void set_planes(float near, float far) { 
        m_near_plane = near;
        m_far_plane = far;
    }

	glm::vec3 velocity {0.f, 0.f, 0.f};
	glm::vec3 position {0.f, 0.f, 0.f};
	float pitch {0.f};
	float yaw {0.f};

	glm::mat4 get_view_matrix();
	glm::mat4 get_rotation_matrix();
    glm::mat4 get_projection_matrix();

	void update(const InputManager& input, float delta_time);
private:
    float m_fov;
    float m_near_plane;
    float m_far_plane;
    float m_aspect_ratio;
};
