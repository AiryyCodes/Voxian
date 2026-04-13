#include "Input.h"
#include "Window.h"
#include <GLFW/glfw3.h>

Input::Input(Window &window)
    : m_Window(&window)
{
    glfwSetWindowUserPointer(m_Window->GetHandle(), this);
    glfwSetKeyCallback(m_Window->GetHandle(), KeyCallback);
    glfwSetMouseButtonCallback(m_Window->GetHandle(), MouseButtonCallback);
}

Input::~Input()
{
    if (!m_Window)
        return;

    glfwSetKeyCallback(m_Window->GetHandle(), nullptr);
    glfwSetWindowUserPointer(m_Window->GetHandle(), nullptr);
    glfwSetMouseButtonCallback(m_Window->GetHandle(), nullptr);
}

void Input::Update()
{
    for (auto &key : m_Keys)
    {
        key.Previous = key.Current;
        key.IsDoubleTapped = false;
    }

    for (auto &button : m_MouseButtons)
    {
        button.Previous = button.Current;
    }
}

bool Input::IsKeyDown(int key) const
{
    if (key < 0 || key > GLFW_KEY_LAST)
        return false;

    return m_Keys[key].Current;
}

bool Input::IsKeyJustDown(int key) const
{
    if (key < 0 || key > GLFW_KEY_LAST)
        return false;

    return m_Keys[key].Current && !m_Keys[key].Previous;
}

bool Input::IsKeyJustReleased(int key) const
{
    if (key < 0 || key > GLFW_KEY_LAST)
        return false;

    return !m_Keys[key].Current && m_Keys[key].Previous;
}

bool Input::IsKeyDoubleTapped(int key) const
{
    if (key < 0 || key > GLFW_KEY_LAST)
        return false;

    return m_Keys[key].IsDoubleTapped;
}

float Input::GetMouseX() const
{
    double x, y;
    glfwGetCursorPos(m_Window->GetHandle(), &x, &y);
    return static_cast<float>(x);
}

float Input::GetMouseY() const
{
    double x, y;
    glfwGetCursorPos(m_Window->GetHandle(), &x, &y);
    return static_cast<float>(y);
}

bool Input::IsMouseButtonPressed(int button)
{
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
        return false;

    return m_MouseButtons[button].Current && !m_MouseButtons[button].Previous;
}

bool Input::IsMouseButtonJustPressed(int button)
{
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
        return false;

    return m_MouseButtons[button].Current && !m_MouseButtons[button].Previous;
}

void Input::SetCursorMode(int mode) const
{
    glfwSetInputMode(m_Window->GetHandle(), GLFW_CURSOR, mode);
}

bool Input::IsCursorLocked() const
{
    return glfwGetInputMode(m_Window->GetHandle(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
}

Input *Input::Get(GLFWwindow *window)
{
    return static_cast<Input *>(glfwGetWindowUserPointer(window));
}

void Input::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto *self = Get(window);
    if (!self || key < 0 || key > GLFW_KEY_LAST)
        return;

    if (action == GLFW_PRESS)
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - self->m_Keys[key].LastPressTime;

        // Standard double-tap threshold is ~0.25 seconds
        if (deltaTime < 0.25)
        {
            // Trigger Double Tap Logic
            self->m_Keys[key].IsDoubleTapped = true;

            // Set to a very small value to prevent a "triple tap"
            // from counting as two double taps
            self->m_Keys[key].LastPressTime = -100.0;
        }
        else
        {
            self->m_Keys[key].LastPressTime = currentTime;
            self->m_Keys[key].IsDoubleTapped = false;
        }

        self->m_Keys[key].Current = true;
    }

    if (action == GLFW_RELEASE)
    {
        self->m_Keys[key].Current = false;
    }
}

void Input::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    auto *self = Get(window);
    if (!self || button < 0 || button > GLFW_KEY_LAST)
        return;

    if (action == GLFW_PRESS)
    {
        self->m_MouseButtons[button].Current = true;
    }

    if (action == GLFW_RELEASE)
    {
        self->m_MouseButtons[button].Current = false;
    }
}