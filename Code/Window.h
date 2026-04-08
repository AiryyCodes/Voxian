#pragma once

#include <GLFW/glfw3.h>
#include <string>

class Window
{
public:
    Window(const std::string &title, int width, int height);

    bool IsClosing() const { return glfwWindowShouldClose(m_Handle); }

    int GetWidth() const;
    int GetHeight() const;

    GLFWwindow *GetHandle() const { return m_Handle; }

private:
    GLFWwindow *m_Handle;
};
