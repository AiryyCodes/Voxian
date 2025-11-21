#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstdlib>

#include "Graphics/Camera.h"
#include "Graphics/Shader.h"
#include "Graphics/Window.h"
#include "Logger.h"
#include "Time.h"
#include "World/Block.h"
#include "World/ChunkManager.h"

int main()
{
    auto formatter = CreateScope<spdlog::pattern_formatter>();
    // formatter->add_flag<ThreadNameFlag>('N');
    // formatter->set_pattern("[%T] [%N] [%^%l%$]: %v");
    formatter->set_pattern("[%T] [%^%l%$]: %v");

    auto logger = Logger::GetConsoleLogger();
    logger->set_formatter(std::move(formatter));

    if (!glfwInit())
    {
        LOG_ERROR("Failed to initialize GLFW!");
        return EXIT_FAILURE;
    }

    Window window(1280, 720, "Voxian");

    glfwMakeContextCurrent(window.GetGLFWWindow());

    int version = gladLoadGL(glfwGetProcAddress);
    if (!version)
    {
        LOG_ERROR("Failed to load OpenGL");
        return EXIT_FAILURE;
    }

    LOG_INFO("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    Shader shader;
    shader.Init("Assets/Shaders/Main.vert", "Assets/Shaders/Main.frag");

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Input::SetCursorMode(CursorMode::Locked);

    g_BlockRegistry.Init();

    Camera camera;

    ChunkManager chunkManager;

    while (!window.IsClosing())
    {
        Time::Update();

        float fps = (Time::GetDelta() > 0.0f) ? (1.0f / static_cast<float>(Time::GetDelta())) : 0.0f;

        window.PollEvents();

        camera.Update();

        chunkManager.UpdatePlayerPosition(camera.GetPosition());

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.4f, 0.6f, 0.7f, 1.0f);

        glViewport(0, 0, window.GetFrameBufferWidth(), window.GetFrameBufferHeight());

        shader.Bind();
        shader.SetUniform("u_View", camera.GetMatrices().view);
        shader.SetUniform("u_Projection", camera.GetMatrices().projection);

        chunkManager.Update(shader);

        window.SwapBuffers();

        // LOG_INFO("FPS: {}", fps);
    }

    // glfwTerminate();

    return EXIT_SUCCESS;
}
