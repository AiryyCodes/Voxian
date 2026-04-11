#include "World/Entity/Player.h"
#include "Engine.h"
#include "Input.h"
#include "Math/Vector.h"
#include "Physics/AABB.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include "World/Entity/Component/EntityPhysics.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Component/Camera.h"
#include "World/Entity/Entity.h"
#include <GLFW/glfw3.h>

Player::Player()
    : Entity("Player")
{
    auto &transform = AddComponent<Transform>();
    transform.Position = Vector3f(CHUNK_SIZE / 2.0f, CHUNK_HEIGHT - 64 + 32.0f, CHUNK_SIZE / 2.0f);

    Input &input = EngineContext::GetInput();
    input.SetCursorMode(GLFW_CURSOR_DISABLED);

    AddComponent<PlayerInput>();
    AddComponent<EntityPhysics>();

    AddComponent<Camera>(70.0f);
}

void PlayerInput::OnUpdate(float delta)
{
    auto &transform = GetOwner().GetComponent<Transform>();
    auto &physics = GetOwner().GetComponent<EntityPhysics>();
    Input &input = EngineContext::GetInput();

    // Get forward/right from player yaw only (ignore pitch for movement)
    glm::vec3 localForward(0.f, 0.f, -1.f);
    glm::vec3 localRight(1.f, 0.f, 0.f);
    Matrix4 transformMatrix = transform.GetMatrix();
    glm::vec3 forward = glm::normalize(glm::vec3(transformMatrix * glm::vec4(localForward, 0.f)));
    glm::vec3 right = glm::normalize(glm::vec3(transformMatrix * glm::vec4(localRight, 0.f)));

    // Flatten to ground plane
    forward.y = 0.f;
    forward = glm::normalize(forward);
    right.y = 0.f;
    right = glm::normalize(right);

    // Build XZ movement direction
    glm::vec3 movement(0.f);
    if (input.IsKeyDown(GLFW_KEY_W))
        movement += forward;
    if (input.IsKeyDown(GLFW_KEY_S))
        movement -= forward;
    if (input.IsKeyDown(GLFW_KEY_A))
        movement -= right;
    if (input.IsKeyDown(GLFW_KEY_D))
        movement += right;

    // Apply XZ velocity directly (friction in EntityPhysics handles deceleration)
    if (glm::length(movement) > 0.f)
    {
        glm::vec3 dir = glm::normalize(movement);
        physics.SetVelocity(Vector3f(
            dir.x * m_Speed,
            physics.GetVelocity().y, // preserve Y so gravity isn't wiped
            dir.z * m_Speed));
    }

    // Jump
    if (input.IsKeyDown(GLFW_KEY_SPACE) && physics.IsOnGround())
        physics.SetVelocity(Vector3f(physics.GetVelocity().x, m_JumpForce, physics.GetVelocity().z));

    // Mouse look
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
    float xOffset = (xPos - lastX) * 0.1f;
    float yOffset = (lastY - yPos) * 0.1f;
    lastX = xPos;
    lastY = yPos;

    transform.Rotation.y -= xOffset;

    auto &camera = GetOwner().GetComponent<Camera>();
    float pitch = glm::clamp(camera.GetPitch() + yOffset, -89.f, 89.f);
    camera.SetPitch(pitch);
}

AABB Player::GetAABB() const
{
    const Transform &transform = GetComponent<Transform>();
    return {
        transform.Position + Vector3f(-0.3f, 0.0f, -0.3f),
        transform.Position + Vector3f(0.3f, 1.8f, 0.3f),
    };
}
