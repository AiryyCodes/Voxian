#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "Logger.h"
#include "Renderer/Mesh.h"
#include "Window.h"

Renderer::~Renderer()
{
    m_Shaders = {};

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
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    m_Shaders.Init();

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

void Renderer::SetViewport(int x, int y, int width, int height) const
{
    glViewport(x, y, width, height);
}

void Renderer::Submit(const Mesh &mesh)
{
    glBindVertexArray(mesh.GetVAO());

    if (mesh.GetTexture())
    {
        mesh.GetTexture()->Bind();
    }

    if (mesh.HasIndices())
    {
        glDrawElements(GL_TRIANGLES, mesh.GetNumIndices(), GL_UNSIGNED_INT, nullptr);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, mesh.GetNumVertices());
    }
    glBindVertexArray(0);
}
