#include "Window.h"
#include "Logger.h"

#include <GLFW/glfw3.h>

Window::Window(const std::string &title, int width, int height)
{
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_Handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Handle)
    {
        LOG_ERROR("Failed to create window!");
        return;
    }
}

int Window::GetWidth() const
{
    int width, height;
    glfwGetFramebufferSize(m_Handle, &width, &height);
    return width;
}

int Window::GetHeight() const
{
    int width, height;
    glfwGetFramebufferSize(m_Handle, &width, &height);
    return height;
}