#include "Engine.h"
#include "Input.h"
#include "Logger.h"
#include "Memory.h"
#include "Window.h"

bool Engine::Init()
{
    if (!glfwInit())
    {
        LOG_FATAL("Failed to initialize GLFW!");
        return false;
    }

    m_Window = CreateScope<Window>("Voxian", 1280, 720);

    if (!m_Renderer.Init(*m_Window))
        return false;

    m_Input = CreateScope<Input>(*m_Window);

    return true;
}

bool Engine::IsRunning() const
{
    return !m_Window->IsClosing();
}
