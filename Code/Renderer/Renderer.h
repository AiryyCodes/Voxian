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

    void Clear(int flags) const;
    void ClearColor(float r, float g, float b, float a) const;

private:
    GLFWwindow *m_WindowHandle = nullptr;
};
