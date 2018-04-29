#include "Camera.h"
#include <glm/gtx/rotate_vector.hpp>

Camera::Camera(float width, float height)
{
	m_pos = glm::vec3(0.0f, 0.5f, -5.0f);
	m_dir = glm::vec3(0.0f, 0.0f, 1.0f);
	m_up = glm::vec3(0.0f, -1.0f, 0.0f);
	m_rot = glm::vec3(0.0f, 0.0f, 0.0f);

	m_view = glm::lookAt(m_pos, m_pos + m_dir, m_up);
	m_proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10.0f);
}

Camera::~Camera()
{
}

void Camera::Update()
{
	
}