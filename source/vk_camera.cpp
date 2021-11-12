#include "vk_camera.h"


Camera::Camera() :
	sphericalCoord(SphericalCoord{}), fov(0.0f), aspectRatio(1.0f), nearClip(0.1f), farClip(10.0f), up(glm::vec3(0.0f, 1.0f, 0.0f))
{
}

Camera::Camera(SphericalCoord sphericalCoord, float fov, float aspectRatio, float nearClip, float farClip, glm::vec3 up) :
	sphericalCoord(sphericalCoord), fov(fov), aspectRatio(aspectRatio), nearClip(nearClip), farClip(farClip), up(up) {
	position = toCartesian();
}


void Camera::rotate(float dTheta, float dPhi, float rotFactor) {
	sphericalCoord.m_theta += dTheta * rotFactor;
	sphericalCoord.m_phi = glm::clamp(sphericalCoord.m_phi + dPhi * rotFactor, 0.0001f, glm::radians(179.999f));
	position = toCartesian();
}

void Camera::zoom(float distance, float zoomFactor) {
	sphericalCoord.m_radius = glm::clamp(sphericalCoord.m_radius - distance * zoomFactor, nearClip, farClip);
	position = toCartesian();
}


glm::vec3 Camera::toCartesian() {
	float x = sphericalCoord.m_radius * sinf(sphericalCoord.m_phi) * sinf(sphericalCoord.m_theta);
	float y = sphericalCoord.m_radius * cosf(sphericalCoord.m_phi);
	float z = sphericalCoord.m_radius * sinf(sphericalCoord.m_phi) * cosf(sphericalCoord.m_theta);
	return glm::vec3(x, y, z) + sphericalCoord.m_target;
}

glm::mat4 Camera::viewMatrix()
{
	return glm::lookAt(position, sphericalCoord.m_target, up);
}

glm::mat4 Camera::projMatrix()
{
	return glm::perspective(fov, aspectRatio, nearClip, farClip);
}
