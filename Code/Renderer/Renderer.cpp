#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "Logger.h"
#include "Renderer/Mesh.h"
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
        LOG_FATAL("Failed to initialize OpenGL context!");
        return false;
    }

    LOG_INFO("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    glEnable(GL_MULTISAMPLE);

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

void Renderer::Clear(int flags) const
{
    glClear(flags);
}

void Renderer::ClearColor(float r, float g, float b, float a) const
{
    glClearColor(r, g, b, a);
}

void Renderer::Submit(const Mesh &mesh)
{
    glBindVertexArray(mesh.GetVAO());
    glDrawArrays(GL_TRIANGLES, 0, mesh.GetNumVertices());
    glBindVertexArray(0);
}
