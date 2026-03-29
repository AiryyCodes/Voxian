#include "World/Entity/Player.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"

Player::Player()
    : Entity("Player")
{
    auto &transform = AddComponent<Transform>();
    transform.Position = Vector3f(0.0f, 0.0f, 0.0f);
}
