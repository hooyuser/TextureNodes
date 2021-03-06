#include "vk_camera.h"

void Camera::rotate(const float d_theta, const float d_phi, const float rot_factor) {
	spherical_coord.theta += d_theta * rot_factor;
	spherical_coord.phi = glm::clamp(spherical_coord.phi + d_phi * rot_factor, 0.0001f, glm::radians(179.999f));
	position = to_cartesian();
}

void Camera::zoom(const float distance, const float zoom_factor) {
	spherical_coord.radius = glm::clamp(spherical_coord.radius - distance * zoom_factor, near_clip, far_clip);
	position = to_cartesian();
}

glm::vec3 Camera::to_cartesian() const {
	const float x = spherical_coord.radius * sinf(spherical_coord.phi) * sinf(spherical_coord.theta);
	const float y = spherical_coord.radius * cosf(spherical_coord.phi);
	const float z = spherical_coord.radius * sinf(spherical_coord.phi) * cosf(spherical_coord.theta);
	return glm::vec3(x, y, z) + spherical_coord.target;
}