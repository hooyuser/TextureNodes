#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct SphericalCoord {
	float theta;
	float phi;
	float radius;
	glm::vec3 target;
};

class Camera {
private:
	SphericalCoord spherical_coord;
	glm::vec3 position;
	glm::vec3 up;
	float fov;
	float aspect_ratio;
	float near_clip;
	float far_clip;

public:
	Camera() :
		spherical_coord(SphericalCoord{}), up(glm::vec3(0.0f, 1.0f, 0.0f)), fov(0.0f), aspect_ratio(1.0f), near_clip(0.1f), far_clip(10.0f) {}

	Camera(SphericalCoord spherical_coordinate, float fov, float aspect_ratio, float near_clip, float far_clip, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f)) :
		spherical_coord(spherical_coordinate), up(up), fov(fov), aspect_ratio(aspect_ratio), near_clip(near_clip), far_clip(far_clip) {
		position = to_cartesian();
	}

	void set_aspect_ratio(const float cam_aspect_ratio) {
		aspect_ratio = cam_aspect_ratio;
	}

	[[nodiscard]] glm::vec3 get_position() const {
		return position;
	}

	void rotate(float d_theta, float d_phi, float rot_factor = 1.0f);

	void zoom(float distance, float zoom_factor = 1.0f);

	[[nodiscard]] glm::vec3 to_cartesian() const;

	[[nodiscard]] glm::mat4 view_matrix() const {
		return glm::lookAt(position, spherical_coord.target, up);
	}

	[[nodiscard]] glm::mat4 proj_matrix() const {
		return glm::perspective(fov, aspect_ratio, near_clip, far_clip);
	}
};
