#include "Graphics/Camera.h"
#include "Graphics/Window.h"
#include "Logger.h"
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

    // LOG_INFO("Forward: X: {} Y: {} Z: {}", m_Forward.x, m_Forward.y, m_Forward.z);

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

    // LOG_INFO("Position: X: {} Y: {} Z: {}", m_Position.x, m_Position.y, m_Position.z);

    float mouseDeltaX = Input::GetMouseDeltaX() * m_Sensitivity;
    float mouseDeltaY = Input::GetMouseDeltaY() * m_Sensitivity;

    Vector3 rotation = m_Rotation;

    float pitch = rotation.x + mouseDeltaY;
    float yaw = rotation.y - mouseDeltaX;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    m_Rotation = Vector3(pitch, yaw, 0.0f);

    glm::mat4 rotationMatrix = glm::eulerAngleYXZ(
        glm::radians(m_Rotation.y),
        glm::radians(m_Rotation.x),
        glm::radians(m_Rotation.z));

    m_Forward = glm::vec3(rotationMatrix * glm::vec4(0, 0, -1, 0));

    Vector3f up = {0.0f, 1.0f, 0.0f};
    Vector3f center = m_Position + m_Forward;

    m_Right = glm::normalize(glm::cross(m_Forward, up));

    m_Matrices.view = glm::lookAt(m_Position, center, up);

    Window &window = Window::GetMain();
    float aspect = (float)window.GetWidth() / (float)window.GetHeight();

    m_Matrices.projection = glm::perspective(glm::radians(m_Fov), aspect, m_Near, m_Far);

    // LOG_INFO("Camera X: {} Y: {} Z: {}", m_Position.x, m_Position.y, m_Position.z);
    // LOG_INFO("Camera Rotation X: {} Y: {} Z: {}", m_Rotation.x, m_Rotation.y, m_Rotation.z);
}

void Camera::Draw(const Shader &shader, const Matrices &matrices) const
{
    shader.Bind();
    shader.SetUniform("u_View", matrices.view);
    shader.SetUniform("u_Projection", matrices.projection);
}
