#include "Graphics/Camera.h"
#include "Graphics/Window.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "MyTime.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

void Camera::Update()
{
    float speed = m_Speed * Time::GetDelta();

    if (Input::IsKeyHeld(GLFW_KEY_W))
    {
        m_Position += m_Forward * speed;
    }
    if (Input::IsKeyHeld(GLFW_KEY_S))
    {
        m_Position -= m_Forward * speed;
    }

    if (Input::IsKeyHeld(GLFW_KEY_A))
    {
        m_Position -= m_Right * speed;
    }
    if (Input::IsKeyHeld(GLFW_KEY_D))
    {
        m_Position += m_Right * speed;
    }

    float mouseDeltaX = Input::GetMouseDeltaX() * m_Sensitivity;
    float mouseDeltaY = Input::GetMouseDeltaY() * m_Sensitivity;

    Vector3f rotation = m_Rotation;

    float pitch = rotation.x + mouseDeltaY;
    float yaw = rotation.y - mouseDeltaX;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    m_Rotation = Vector3f(pitch, yaw, 0.0f);

    Matrix4 rotationMatrix = glm::eulerAngleYXZ(
        glm::radians(m_Rotation.y),
        glm::radians(m_Rotation.x),
        glm::radians(m_Rotation.z));

    m_Forward = Vector3f(rotationMatrix * Vector4(0, 0, -1, 0));

    Vector3f up = {0.0f, 1.0f, 0.0f};
    Vector3f center = m_Position + m_Forward;

    m_Right = glm::normalize(glm::cross(m_Forward, up));

    m_Matrices.View = glm::lookAt(m_Position, center, up);

    Window &window = Window::GetMain();
    float aspect = (float)window.GetWidth() / (float)window.GetHeight();

    m_Matrices.Projection = glm::perspective(glm::radians(m_Fov), aspect, m_Near, m_Far);
}

void Camera::Draw(const Shader &shader, const Matrices &matrices) const
{
    shader.Bind();
    shader.SetUniform("u_View", matrices.View);
    shader.SetUniform("u_Projection", matrices.Projection);
}

Camera::Frustum Camera::GetFrustum() const
{
    Frustum frustum;
    Matrix4 matrix = m_Matrices.Projection * m_Matrices.View;

    // Left
    frustum.Planes[0].Normal.x = matrix[0][3] + matrix[0][0];
    frustum.Planes[0].Normal.y = matrix[1][3] + matrix[1][0];
    frustum.Planes[0].Normal.z = matrix[2][3] + matrix[2][0];
    frustum.Planes[0].D = matrix[3][3] + matrix[3][0];

    // Right
    frustum.Planes[1].Normal.x = matrix[0][3] - matrix[0][0];
    frustum.Planes[1].Normal.y = matrix[1][3] - matrix[1][0];
    frustum.Planes[1].Normal.z = matrix[2][3] - matrix[2][0];
    frustum.Planes[1].D = matrix[3][3] - matrix[3][0];

    // Bottom
    frustum.Planes[2].Normal.x = matrix[0][3] + matrix[0][1];
    frustum.Planes[2].Normal.y = matrix[1][3] + matrix[1][1];
    frustum.Planes[2].Normal.z = matrix[2][3] + matrix[2][1];
    frustum.Planes[2].D = matrix[3][3] + matrix[3][1];

    // Top
    frustum.Planes[3].Normal.x = matrix[0][3] - matrix[0][1];
    frustum.Planes[3].Normal.y = matrix[1][3] - matrix[1][1];
    frustum.Planes[3].Normal.z = matrix[2][3] - matrix[2][1];
    frustum.Planes[3].D = matrix[3][3] - matrix[3][1];

    // Near
    frustum.Planes[4].Normal.x = matrix[0][3] + matrix[0][2];
    frustum.Planes[4].Normal.y = matrix[1][3] + matrix[1][2];
    frustum.Planes[4].Normal.z = matrix[2][3] + matrix[2][2];
    frustum.Planes[4].D = matrix[3][3] + matrix[3][2];

    // Far
    frustum.Planes[5].Normal.x = matrix[0][3] - matrix[0][2];
    frustum.Planes[5].Normal.y = matrix[1][3] - matrix[1][2];
    frustum.Planes[5].Normal.z = matrix[2][3] - matrix[2][2];
    frustum.Planes[5].D = matrix[3][3] - matrix[3][2];

    // Normalize Planes
    for (int i = 0; i < 6; i++)
    {
        float length = glm::length(frustum.Planes[i].Normal);
        frustum.Planes[i].Normal /= length;
        frustum.Planes[i].D /= length;
    }

    return frustum;
}

bool Camera::IsInsideFrustum(const Frustum &frustum, const AABB &box) const
{
    for (int i = 0; i < 6; i++)
    {
        const auto &p = frustum.Planes[i];

        glm::vec3 positive = box.Min;

        if (p.Normal.x >= 0)
            positive.x = box.Max.x;
        if (p.Normal.y >= 0)
            positive.y = box.Max.y;
        if (p.Normal.z >= 0)
            positive.z = box.Max.z;

        if (glm::dot(p.Normal, positive) + p.D < 0)
            return false;
    }
    return true;
}
