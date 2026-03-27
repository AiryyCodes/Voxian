#include "Engine.h"
#include "Logger.h"
#include "Renderer/Renderer.h"

#include <GLFW/glfw3.h>
#include <cstdlib>

int main()
{
    Logger::Init();

    Engine engine;
    if (!engine.Init())
    {
        LOG_FATAL("Failed to initialize engine!");
        return EXIT_FAILURE;
    }

    EngineContext::Init(&engine);

    Renderer &renderer = engine.GetRenderer();

    while (engine.IsRunning())
    {
        glfwPollEvents();

        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.ClearColor(0.4f, 0.7f, 0.9f, 1.0f);

        renderer.SwapBuffers();
    }

    LOG_INFO("Shutting down...");

    EngineContext::Shutdown();

    return EXIT_SUCCESS;
}
