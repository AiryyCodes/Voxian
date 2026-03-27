#include <cstdio>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "Window.h"

Renderer::~Renderer()
{
    glfwTerminate();
}

bool Renderer::Init(const Window &window)
{
    m_WindowHandle = window.GetHandle();

    MakeCurrent();

    int version = gladLoadGL(glfwGetProcAddress);
    if (!version)
    {
        printf("Failed to initialize OpenGL context!");
        return false;
    }

    printf("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    return true;
}

void Renderer::MakeCurrent() const
{
    glfwMakeContextCurrent(m_WindowHandle);
}

void Renderer::SwapBuffers() const
{
    glfwSwapBuffers(m_WindowHandle);
}
