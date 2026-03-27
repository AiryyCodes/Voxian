#include "Window.h"
#include "Logger.h"

#include <GLFW/glfw3.h>

Window::Window(const std::string &title, int width, int height)
{
    glfwDefaultWindowHints();

    m_Handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Handle)
    {
        LOG_ERROR("Failed to create window!");
        return;
    }
}
