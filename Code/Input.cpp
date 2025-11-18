#include "Input.h"
#include "Graphics/Window.h"

std::array<bool, static_cast<size_t>(GLFW_KEY_LAST)> Input::m_Keys = {};
std::array<bool, static_cast<size_t>(GLFW_KEY_LAST)> Input::m_KeysHeld = {};

float Input::m_MouseX = 0.0f;
float Input::m_MouseY = 0.0f;
float Input::m_MouseLastX = 0.0f;
float Input::m_MouseLastY = 0.0f;

bool Input::m_FirstMouse = true;

CursorMode Input::m_CursorMode = CursorMode::Unlocked;

void Input::OnKey(int key, int action)
{
    if (static_cast<size_t>(key) < static_cast<size_t>(GLFW_KEY_LAST))
    {
        m_Keys[static_cast<size_t>(key)] = action == GLFW_PRESS;
        m_KeysHeld[static_cast<size_t>(key)] = action == GLFW_PRESS || action == GLFW_REPEAT;
    }
}

bool Input::IsKeyDown(int key)
{
    return m_Keys[static_cast<size_t>(key)];
}

bool Input::IsKeyHeld(int key)
{
    return m_KeysHeld[static_cast<size_t>(key)];
}

void Input::OnMouseMove(float mouseX, float mouseY)
{
    m_MouseX = mouseX;
    m_MouseY = mouseY;

    if (m_FirstMouse)
    {
        m_MouseLastX = m_MouseX;
        m_MouseLastY = m_MouseY;
        m_FirstMouse = false;
    }
}

float Input::GetMouseX()
{
    return m_MouseX;
}

float Input::GetMouseY()
{
    return m_MouseY;
}

float Input::GetMouseDeltaX()
{
    float deltaX = m_MouseX - m_MouseLastX;
    m_MouseLastX = m_MouseX;
    return deltaX;
}

float Input::GetMouseDeltaY()
{
    float deltaY = m_MouseLastY - m_MouseY;
    m_MouseLastY = m_MouseY;
    return deltaY;
}

void Input::SetCursorMode(CursorMode mode)
{
    Window &window = Window::GetMain();
    window.SetCursorMode(mode);
}

bool Input::IsCursorLocked()
{
    return m_CursorMode == CursorMode::Locked;
}
