#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Shader.h"
#include "Renderer/ShaderLibrary.h"
#include "Window.h"

#include <GLFW/glfw3.h>
#include <string>

class Renderer
{
public:
    ~Renderer();

    bool Init(const Window &window);

    void MakeCurrent() const;
    void SwapBuffers() const;

    void Clear(int flags) const;
    void ClearColor(float r, float g, float b, float a) const;
    void SetViewport(int x, int y, int width, int height) const;

    void Submit(const Mesh &mesh);

    template <typename Func>
    void Submit(const Mesh &mesh, const std::string &shaderName, Func &&setUniforms)
    {
        Shader &shader = m_Shaders.Get(shaderName);
        shader.Bind();
        setUniforms(shader);

        glBindVertexArray(mesh.GetVAO());
        glDrawArrays(GL_TRIANGLES, 0, mesh.GetNumVertices());
        glBindVertexArray(0);
    }

private:
    GLFWwindow *m_WindowHandle = nullptr;

    ShaderLibrary m_Shaders;
};
