#include "World/Entity/Entity.h"
#include "World/Entity/Component/Camera.h"
#include "World/Entity/Component/Transform.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

Matrix4 Camera::GetViewMatrix() const
{
    auto &transform = GetOwner().GetComponent<Transform>();

    Matrix4 view(1.0f);

    // Apply rotations: yaw from player, pitch from camera
    float yaw = glm::radians(transform.Rotation.y);
    float pitch = glm::radians(m_Pitch);

    // Rotate around the X axis for pitch, then around the Y axis for yaw
    view = glm::rotate(view, -pitch, Vector3f(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, -yaw, Vector3f(0.0f, 1.0f, 0.0f));

    // Translate to camera position
    view = glm::translate(view, -transform.Position);

    return view;
}

void Camera::UpdateProjectionMatrix()
{
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
}