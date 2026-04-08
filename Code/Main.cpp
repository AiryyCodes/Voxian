#include "Engine.h"
#include "Logger.h"
#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"
#include "World/World.h"

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

    World world;
    world.Init();

    while (engine.IsRunning())
    {
        renderer.SetViewport(0, 0, engine.GetWindow().GetWidth(), engine.GetWindow().GetHeight());

        engine.GetInput().Update();

        glfwPollEvents();

        world.Update(0.0f);

        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.ClearColor(0.4f, 0.7f, 0.9f, 1.0f);

        world.Render(renderer);

        Shader::Unbind();

        renderer.SwapBuffers();
    }

    LOG_INFO("Shutting down...");

    EngineContext::Shutdown();

    return EXIT_SUCCESS;
}
