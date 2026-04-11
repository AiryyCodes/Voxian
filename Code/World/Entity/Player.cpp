#include "World/Entity/Player.h"
#include "Engine.h"
#include "Input.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Component/Camera.h"
#include "World/Entity/Entity.h"
#include <GLFW/glfw3.h>

Player::Player()
    : Entity("Player")
{
    auto &transform = AddComponent<Transform>();
    transform.Position = Vector3f(CHUNK_SIZE / 2.0f, CHUNK_HEIGHT - 64 + 4.0f, CHUNK_SIZE / 2.0f);

    Input &input = EngineContext::GetInput();
    input.SetCursorMode(GLFW_CURSOR_DISABLED);

    AddComponent<PlayerInput>();
    AddComponent<Camera>(70.0f);
}

void PlayerInput::OnUpdate(float delta)
{
    auto &transform = GetOwner().GetComponent<Transform>();

    Input &input = EngineContext::GetInput();

    // Transform local directions through the camera's rotation matrix to get world directions
    // This ensures movement matches the actual camera orientation
    glm::vec3 localForward(0.f, 0.f, -1.f);
    glm::vec3 localRight(1.f, 0.f, 0.f);
    glm::vec3 localUp(0.f, 1.f, 0.f);

    Matrix4 transformMatrix = transform.GetMatrix();
    glm::vec3 forward = glm::normalize(glm::vec3(transformMatrix * glm::vec4(localForward, 0.f)));
    glm::vec3 right = glm::normalize(glm::vec3(transformMatrix * glm::vec4(localRight, 0.f)));
    glm::vec3 up = glm::normalize(glm::vec3(transformMatrix * glm::vec4(localUp, 0.f)));

    // Flatten forward to ground plane
    forward.y = 0.f;
    forward = glm::normalize(forward);

    glm::vec3 movement(0.f);
    if (input.IsKeyDown(GLFW_KEY_W))
        movement += forward;
    if (input.IsKeyDown(GLFW_KEY_S))
        movement -= forward;
    if (input.IsKeyDown(GLFW_KEY_A))
        movement -= right;
    if (input.IsKeyDown(GLFW_KEY_D))
        movement += right;

    // Go straight up when space is held, and go down when shift is held
    // Not dependent on camera rotation since it's just a simple up/down movement
    if (input.IsKeyDown(GLFW_KEY_SPACE))
        movement += localUp;
    if (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT))
        movement -= localUp;

    if (glm::length(movement) > 0.f)
        transform.Position += glm::normalize(movement) * m_Speed * delta;

    // Camera rotation with mouse
    static bool firstMouse = true;
    static float lastX = 400.f, lastY = 300.f;

    float xPos = input.GetMouseX();
    float yPos = input.GetMouseY();

    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos; // Reversed since y-coordinates go from bottom to top

    lastX = xPos;
    lastY = yPos;

    float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    // Update player yaw (horizontal rotation)
    transform.Rotation.y -= xOffset;

    // Update camera pitch (vertical rotation)
    auto &camera = GetOwner().GetComponent<Camera>();
    camera.SetPitch(camera.GetPitch() + yOffset);

    // Clamp camera pitch
    float currentPitch = camera.GetPitch();
    if (currentPitch > 89.0f)
        camera.SetPitch(89.0f);
    if (currentPitch < -89.0f)
        camera.SetPitch(-89.0f);
}
