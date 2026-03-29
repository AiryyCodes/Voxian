#include "Input.h"
#include "Window.h"
#include <GLFW/glfw3.h>

Input::Input(Window &window)
    : m_Window(&window)
{
    glfwSetWindowUserPointer(m_Window->GetHandle(), this);
    glfwSetKeyCallback(m_Window->GetHandle(), KeyCallback);
}

Input::~Input()
{
    if (!m_Window)
        return;

    glfwSetKeyCallback(m_Window->GetHandle(), nullptr);
    glfwSetWindowUserPointer(m_Window->GetHandle(), nullptr);
}

void Input::Update()
{
    for (auto &key : m_Keys)
        key.Previous = key.Current;
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
        self->m_Keys[key].Current = true;
    if (action == GLFW_RELEASE)
        self->m_Keys[key].Current = false;
}
