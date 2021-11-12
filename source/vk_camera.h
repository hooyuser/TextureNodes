#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct SphericalCoord
{
	float m_theta;
	float m_phi;
	float m_radius;
	glm::vec3 m_target;
};

class Camera {
public:
	SphericalCoord sphericalCoord;
	glm::vec3 position;
	glm::vec3 up;
	float fov;
	float aspectRatio;
	float nearClip;
	float farClip;

	Camera();

	Camera(SphericalCoord sphericalCoord, float fov, float aspectRatio, float nearClip, float farClip, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));

	void rotate(float dTheta, float dPhi, float rotFactor = 1.0f);

	void zoom(float distance, float zoomFactor = 1.0f);

	glm::vec3 toCartesian();

	glm::mat4 viewMatrix();

	glm::mat4 projMatrix();
};
