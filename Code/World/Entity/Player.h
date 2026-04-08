#pragma once

#include "World/Entity/Component/Component.h"
#include "World/Entity/Entity.h"

class PlayerInput : public Component
{
public:
    void OnUpdate(float delta) override;

private:
    float m_Speed = 2.5f;
};

class Player : public Entity
{
public:
    Player();
};
