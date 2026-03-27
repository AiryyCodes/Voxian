#include "Engine.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>

int main()
{
    Engine engine;
    if (!engine.Init())
    {
        printf("Failed to initialize engine!");
        return EXIT_FAILURE;
    }

    EngineContext::Init(&engine);

    while (engine.IsRunning())
    {
        glfwPollEvents();

        engine.GetRenderer().SwapBuffers();
    }

    EngineContext::Shutdown();

    return EXIT_SUCCESS;
}
