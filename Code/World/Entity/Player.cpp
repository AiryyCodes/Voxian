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
#include "Util/Raycast.h"

#include <GLFW/glfw3.h>
#include <cmath>

Player::Player()
    : Entity("Player")
{
    auto &transform = AddComponent<Transform>();
    transform.Position = Vector3f(CHUNK_SIZE / 2.0f + 0.5f, 0.0f, CHUNK_SIZE / 2.0f + 0.5f);

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
        int localX = static_cast<int>(transform.Position.x) % CHUNK_SIZE;
        int localZ = static_cast<int>(transform.Position.z) % CHUNK_SIZE;
        localX = (localX + CHUNK_SIZE) % CHUNK_SIZE;
        localZ = (localZ + CHUNK_SIZE) % CHUNK_SIZE;

        int groundY = chunk->GetTopBlockY(localX, localZ);

        // Place player cleanly on top of the surface block
        transform.Position.y = static_cast<float>(groundY);

        // Reset velocity so physics starts clean
        GetOwner().GetComponent<EntityPhysics>().SetVelocity(Vector3f(0.0f));

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
    glm::vec3 localForward(0.0f, 0.0f, -1.0f);
    glm::vec3 localRight(1.0f, 0.0f, 0.0f);
    glm::vec3 localUp(0.0f, 1.0f, 0.0f);
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
    bool sprinting = input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) && !physics.IsFlying() && glm::length(movement) > 0.f;

    float targetSpeed = m_Speed;
    if (physics.IsFlying())
        targetSpeed = m_FlightSpeed;
    else if (sprinting)
        targetSpeed = m_SprintSpeed;

    float currentAccel, currentDecel;
    if (onGround && !physics.IsFlying())
    {
        currentAccel = m_GroundAccel;
        currentDecel = m_GroundDecel;
    }
    else
    {
        currentAccel = m_AirAccel;
        currentDecel = m_AirAccel;
    }

    if (glm::length(movement) > 0.f)
    {
        glm::vec3 dir = glm::normalize(movement);
        // Interpolate toward target velocity
        vel.x = glm::mix(vel.x, dir.x * targetSpeed, glm::min(currentAccel * delta, 1.0f));
        vel.z = glm::mix(vel.z, dir.z * targetSpeed, glm::min(currentAccel * delta, 1.0f));
    }
    else
    {
        if (glm::length(glm::vec2(vel.x, vel.z)) < 0.1f)
        {
            vel.x = 0.0f;
            vel.z = 0.0f;
        }
        else
        {
            vel.x = glm::mix(vel.x, 0.0f, glm::min(currentDecel * delta, 1.0f));
            vel.z = glm::mix(vel.z, 0.0f, glm::min(currentDecel * delta, 1.0f));
        }
    }

    // Jump
    if (input.IsKeyDown(GLFW_KEY_SPACE) && onGround && !physics.IsFlying())
        vel.y = m_JumpForce;

    if (input.IsKeyDoubleTapped(GLFW_KEY_SPACE))
    {
        physics.SetFlying(!physics.IsFlying());
        vel.y = 0.0f;
    }

    if (physics.IsFlying())
    {
        float verticalTarget = 0.0f;

        if (input.IsKeyDown(GLFW_KEY_SPACE))
            verticalTarget = targetSpeed;
        else if (input.IsKeyDown(GLFW_KEY_LEFT_CONTROL))
            verticalTarget = -targetSpeed;

        vel.y = glm::mix(vel.y, verticalTarget, glm::min(currentAccel * delta, 1.0f));
    }
    else if (input.IsKeyDown(GLFW_KEY_SPACE) && onGround)
    {
        // Regular jump remains snappy
        vel.y = m_JumpForce;
    }

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

    // Block placement and destruction
    if (input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        Vector3f origin = transform.Position + Vector3f(0.0f, 1.62f, 0.0f);
        Vector3f dir = camera.GetForward();

        RaycastResult result = Raycast(origin, dir, 6.0f);

        if (result.Hit)
        {
            auto &chunkManager = EngineContext::GetWorld().GetChunkManager();
            chunkManager.SetBlock(result.BlockPos.x, result.BlockPos.y, result.BlockPos.z, 0);
        }
    }

    if (input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        Vector3f origin = transform.Position + Vector3f(0.0f, 1.62f, 0.0f);
        Vector3f dir = camera.GetForward();

        RaycastResult result = Raycast(origin, dir, 6.0f);

        if (result.Hit)
        {
            // Place block in the adjacent position using the face normal
            Vector3i placePos = result.BlockPos + result.Normal;

            // Make sure we're not placing inside the player
            AABB playerBox = GetOwner().GetAABB();
            AABB placeBox = {
                Vector3f(placePos),
                Vector3f(placePos) + 1.0f};

            if (!AABB::Intersects(playerBox, placeBox))
            {
                auto &chunkManager = EngineContext::GetWorld().GetChunkManager();
                chunkManager.SetBlock(placePos.x, placePos.y, placePos.z, Blocks::DIRT_SLAB);
            }
        }
    }
}

AABB Player::GetAABB() const
{
    const Transform &transform = GetComponent<Transform>();
    return {
        transform.Position + Vector3f(-0.3f, 0.0f, -0.3f),
        transform.Position + Vector3f(0.3f, 1.8f, 0.3f),
    };
}
