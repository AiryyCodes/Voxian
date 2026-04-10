#include "Engine.h"
#include "Logger.h"
#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"
#include "Util/Time.h"
#include "World/Block/Blocks.h"
#include "World/World.h"
#include "World/Entity/Component/Camera.h"

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

    engine.GetBlockRegistry().Init();
    Blocks::AssignAll(engine.GetBlockRegistry());

    World &world = engine.GetWorld();
    world.Init();

    while (engine.IsRunning())
    {
        int width = engine.GetWindow().GetWidth();
        int height = engine.GetWindow().GetHeight();
        renderer.SetViewport(0, 0, width, height);

        if (Camera *camera = world.GetActiveCamera())
        {
            camera->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
        }

        Time::Update();
        engine.GetInput().Update();

        glfwPollEvents();

        world.Update(Time::GetDeltaTime());

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
