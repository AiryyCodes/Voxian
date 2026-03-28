#pragma once

#include "Renderer/Mesh.h"
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

    void Submit(const Mesh &mesh);

private:
    GLFWwindow *m_WindowHandle = nullptr;
};
