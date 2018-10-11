#include "Camera.h"
#include <glm/gtx/rotate_vector.hpp>

Camera::Camera(float width, float height)
{
	m_pos = glm::vec3(2.0f, 2.0f, 2.0f);
	m_dir = glm::vec3(-2.0f, -2.0f, -2.0f);
	m_up = glm::vec3(0.0f, 1.0f, 0.0f);
	m_rot = glm::vec3(0.0f, 0.0f, 0.0f);

	m_view = glm::lookAt(m_pos, m_pos + m_dir, m_up);
	m_proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10.0f);
	m_proj[1][1] *= -1;
}

Camera::~Camera()
{
}

void Camera::Update()
{
	m_dir = glm::rotateX(m_dir, m_rot.x);
	m_dir = glm::rotateY(m_dir, m_rot.y);

	m_rot = glm::vec3(0);

	m_view = glm::lookAt(m_pos, m_pos + m_dir, m_up);
}

void Camera::Move(glm::vec3 dir, float speed)
{
	m_pos += dir * speed;
}

void Camera::MoveYAxis(float speed)
{
	m_pos.y += speed;
}

void Camera::ResizeCamera(float width, float height)
{
	m_proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 10.0f);
}

void Camera::MouseRotate(float x, float y)
{
	if (x > -1 && x < 1) m_rot.x = x;
	else if (x < -1)     m_rot.x = -1;
	else if (x > 1)      m_rot.x = 1;

	if (y > -1 && y < 1) m_rot.y = y;
	else if (y < -1)     m_rot.y = -1;
	else if (y > 1)      m_rot.y = 1;
}