#pragma once

#include "Window.h"
#include <GLFW/glfw3.h>

class Renderer
{
public:
    ~Renderer();

    bool Init(const Window &window);

    void MakeCurrent() const;
    void SwapBuffers() const;

private:
    GLFWwindow *m_WindowHandle = nullptr;
};
