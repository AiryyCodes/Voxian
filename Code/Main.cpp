#include "Engine.h"
#include "Logger.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>

static const float TRIANGLE_VERTICES[] = {
    -0.5,
    -0.5f,
    0.0f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    0.5f,
    0.0f,
};

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

    Mesh triangleMesh;
    triangleMesh.Init(TRIANGLE_VERTICES, sizeof(TRIANGLE_VERTICES), {{AttribType::Float3}});

    Shader mainShader("Assets/Shaders/main");

    Matrix4 transform(1.0f);
    transform = glm::translate(transform, Vector3f(0.0f, 0.5f, 0.0f));

    while (engine.IsRunning())
    {
        glfwPollEvents();

        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.ClearColor(0.4f, 0.7f, 0.9f, 1.0f);

        mainShader.Bind();

        mainShader.Set("u_Transform", transform);
        renderer.Submit(triangleMesh);

        Shader::Unbind();

        renderer.SwapBuffers();
    }

    LOG_INFO("Shutting down...");

    EngineContext::Shutdown();

    return EXIT_SUCCESS;
}
