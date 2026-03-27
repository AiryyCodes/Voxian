#include "Window.h"

#include <GLFW/glfw3.h>
#include <cstdio>

Window::Window(const std::string &title, int width, int height)
{
    glfwDefaultWindowHints();

    m_Handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Handle)
    {
        printf("Failed to create window!");
        return;
    }
}
