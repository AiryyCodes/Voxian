#include "World/Entity/Entity.h"
#include "World/Entity/Component/Camera.h"
#include "World/Entity/Component/Transform.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

Matrix4 Camera::GetViewMatrix() const
{
    auto &transform = GetOwner().GetComponent<Transform>();
    Matrix4 viewMatrix = transform.GetMatrix();
    return glm::inverse(viewMatrix);
}

void Camera::UpdateProjectionMatrix()
{
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
}