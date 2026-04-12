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
#include "World/World.h"
#include <GLFW/glfw3.h>
#include <cmath>

Player::Player()
    : Entity("Player")
{
    auto &transform = AddComponent<Transform>();
    transform.Position = Vector3f(CHUNK_SIZE / 2.0f, CHUNK_HEIGHT - 64, CHUNK_SIZE / 2.0f);

    Input &input = EngineContext::GetInput();
    input.SetCursorMode(GLFW_CURSOR_DISABLED);

    AddComponent<PlayerInput>();
    AddComponent<EntityPhysics>();

    AddComponent<SpawnController>();

    AddComponent<Camera>(70.0f);
}

void Player::SetPhysicsEnabled(bool enabled)
{
    auto &physics = GetComponent<EntityPhysics>();
    physics.SetEnabled(enabled);
}

void SpawnController::OnUpdate(float delta)
{
    auto &transform = GetOwner().GetComponent<Transform>();

    World &world = EngineContext::GetWorld();
    auto &chunkManager = world.GetChunkManager();

    int chunkX = static_cast<int>(floor(transform.Position.x / CHUNK_SIZE));
    int chunkZ = static_cast<int>(floor(transform.Position.z / CHUNK_SIZE));

    // Check if the chunk under the player is ready
    if (chunkManager.IsChunkLoaded(chunkX, chunkZ))
    {
        auto chunk = chunkManager.GetChunk(chunkX, chunkZ);

        // Find the ground and place the player
        int groundY = chunk->GetTopBlockY(8, 8);
        transform.Position.y = static_cast<float>(groundY) + 0.0f;

        // Enable physics on the player
        GetOwner().GetComponent<EntityPhysics>().SetEnabled(true);

        Component::Destroy();
    }
    else
    {
        // Keep physics disabled and velocity at zero while waiting
        GetOwner().GetComponent<EntityPhysics>().SetEnabled(false);
    }
}

void PlayerInput::OnUpdate(float delta)
{
    auto &transform = GetOwner().GetComponent<Transform>();
    auto &physics = GetOwner().GetComponent<EntityPhysics>();
    auto &camera = GetOwner().GetComponent<Camera>();
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

    Vector3f vel = physics.GetVelocity();
    bool onGround = physics.IsOnGround();
    bool sprinting = input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) && glm::length(movement) > 0.f;
    float targetSpeed = sprinting ? m_SprintSpeed : m_Speed;

    if (glm::length(movement) > 0.f)
    {
        glm::vec3 dir = glm::normalize(movement);
        float accel = onGround ? 16.0f : 4.0f;
        vel.x = glm::mix(vel.x, dir.x * targetSpeed, glm::min(accel * delta, 1.0f));
        vel.z = glm::mix(vel.z, dir.z * targetSpeed, glm::min(accel * delta, 1.0f));
    }
    else
    {
        float decel = onGround ? 16.0f : 0.0f;
        vel.x = glm::mix(vel.x, 0.0f, glm::min(decel * delta, 1.0f));
        vel.z = glm::mix(vel.z, 0.0f, glm::min(decel * delta, 1.0f));
    }

    // Jump
    if (input.IsKeyDown(GLFW_KEY_SPACE) && onGround)
        vel.y = m_JumpForce;

    physics.SetVelocity(Vector3f(vel.x, vel.y, vel.z));

    // FOV
    float targetFov = sprinting ? camera.GetBaseFOV() + 15.0f : camera.GetBaseFOV();
    camera.SetFOV(glm::mix(camera.GetFOV(), targetFov, delta * 15.0f));

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
