#include "vk_camera.h"

void Camera::rotate(float dTheta, float dPhi, float rotFactor) {
	spherical_coord.theta += dTheta * rotFactor;
	spherical_coord.phi = glm::clamp(spherical_coord.phi + dPhi * rotFactor, 0.0001f, glm::radians(179.999f));
	position = to_cartesian();
}

void Camera::zoom(float distance, float zoomFactor) {
	spherical_coord.radius = glm::clamp(spherical_coord.radius - distance * zoomFactor, near_clip, far_clip);
	position = to_cartesian();
}

glm::vec3 Camera::to_cartesian() const {
	float x = spherical_coord.radius * sinf(spherical_coord.phi) * sinf(spherical_coord.theta);
	float y = spherical_coord.radius * cosf(spherical_coord.phi);
	float z = spherical_coord.radius * sinf(spherical_coord.phi) * cosf(spherical_coord.theta);
	return glm::vec3(x, y, z) + spherical_coord.target;
}