#include "Renderer.h"
#include "Engine.h"
#include "Window.h"

#include <GLFW/glfw3.h>

Renderer::~Renderer()
{
    glfwTerminate();
}

bool Renderer::Init(const Window &window)
{
    glfwMakeContextCurrent(window.GetHandle());

    return true;
}

void Renderer::SwapBuffers() const
{
    Window &window = EngineContext::Get().GetWindow();
    glfwSwapBuffers(window.GetHandle());
}
