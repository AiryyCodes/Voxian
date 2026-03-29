#include "World/Entity/Player.h"
#include "Engine.h"
#include "Input.h"
#include "Logger.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"
#include <GLFW/glfw3.h>

Player::Player()
    : Entity("Player")
{
    auto &transform = AddComponent<Transform>();
    transform.Position = Vector3f(0.0f, 0.0f, 0.0f);

    AddComponent<PlayerInput>();
}

void PlayerInput::OnUpdate(float delta)
{
    auto &transform = GetOwner().GetComponent<Transform>();

    Input &input = EngineContext::GetInput();

    glm::vec3 direction(0.f);
    if (input.IsKeyDown(GLFW_KEY_W))
        direction.y += 1.f;
    if (input.IsKeyDown(GLFW_KEY_S))
        direction.y -= 1.f;
    if (input.IsKeyDown(GLFW_KEY_A))
        direction.x -= 1.f;
    if (input.IsKeyDown(GLFW_KEY_D))
        direction.x += 1.f;

    if (glm::length(direction) > 0.f)
        transform.Position += glm::normalize(direction) * m_Speed;
}
