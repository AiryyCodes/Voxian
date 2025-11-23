#include "Window.h"
#include "Input.h"
#include "Logger.h"

#include <GLFW/glfw3.h>
#include <cstdlib>

Window *Window::m_MainInstance = nullptr;

Window::Window(int width, int height, const std::string &title)
{
    m_Width = width;
    m_Height = height;
    m_Title = title;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Window)
    {
        LOG_ERROR("Failed to create GLFW window.");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(m_Window);

    glfwSetWindowUserPointer(m_Window, (void *)this);

    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *glfwWindow, int width, int height)
                              {
                                Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
                                window->m_Width = width;
                                window->m_Height = height; });

    glfwGetFramebufferSize(m_Window, &m_FrameBufferWidth, &m_FrameBufferHeight);
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow *glfwWindow, int width, int height)
                                   {
                                    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
                                    window->m_FrameBufferWidth = width;
                                    window->m_FrameBufferHeight = height; });

    glfwSetKeyCallback(m_Window, [](GLFWwindow *glfwWindow, int key, int scancode, int action, int mods)
                       { Input::OnKey(key, action); });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow *glfwWindow, double xPos, double yPos)
                             { Input::OnMouseMove(xPos, yPos); });

    if (!m_MainInstance)
        m_MainInstance = this;
}

void Window::SwapBuffers() const
{
    glfwSwapBuffers(m_Window);
}

void Window::SetCursorMode(CursorMode mode)
{
    switch (mode)
    {
    case CursorMode::Hidden:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        break;
    case CursorMode::Locked:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    case CursorMode::Unlocked:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    }
}

bool Window::IsClosing() const
{
    return glfwWindowShouldClose(m_Window);
}

void Window::PollEvents()
{
    glfwPollEvents();
}

void Window::SetTitle(const std::string &title)
{
    m_Title = title;

    glfwSetWindowTitle(m_Window, title.c_str());
}

void Window::SetWidth(int width)
{
    m_Width = width;

    glfwSetWindowSize(m_Window, width, m_Height);
}

void Window::SetHeight(int height)
{
    m_Height = height;

    glfwSetWindowSize(m_Window, m_Width, height);
}
