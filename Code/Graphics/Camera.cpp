#include "Graphics/Camera.h"
#include "Graphics/Window.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Time.h"

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

    m_Matrices.view = glm::lookAt(m_Position, center, up);

    Window &window = Window::GetMain();
    float aspect = (float)window.GetWidth() / (float)window.GetHeight();

    m_Matrices.projection = glm::perspective(glm::radians(m_Fov), aspect, m_Near, m_Far);
}

void Camera::Draw(const Shader &shader, const Matrices &matrices) const
{
    shader.Bind();
    shader.SetUniform("u_View", matrices.view);
    shader.SetUniform("u_Projection", matrices.projection);
}
