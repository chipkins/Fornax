#include "../PrecompiledHeader.h"

class Camera
{
public:
	Camera() {};
	Camera(float width, float height);
	~Camera();

	glm::mat4 getView() { return m_view; }
	glm::mat4 getProj() { return m_proj; }
	glm::vec3 getPos()  { return m_pos; }
	glm::vec3 getDir()  { return m_dir; }
	glm::vec3 getRot()  { return m_rot; }

	void Update();
	void Move(glm::vec3 dir, float speed);
	void MoveYAxis(float speed);
	void ResizeCamera(float width, float height);
	void MouseRotate(float x, float y);

private:
	glm::mat4 m_view;
	glm::mat4 m_proj;
	glm::vec3 m_pos;
	glm::vec3 m_dir;
	glm::vec3 m_up;
	glm::vec3 m_rot;
};