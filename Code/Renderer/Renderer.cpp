#include "Renderer.h"
#include "Window.h"

#include <GLFW/glfw3.h>

Renderer::~Renderer()
{
    glfwTerminate();
}

bool Renderer::Init(const Window &window)
{
    m_WindowHandle = window.GetHandle();

    glfwMakeContextCurrent(m_WindowHandle);

    return true;
}

void Renderer::SwapBuffers() const
{
    glfwSwapBuffers(m_WindowHandle);
}
