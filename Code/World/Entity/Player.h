#pragma once

#include "World/Entity/Component/Component.h"
#include "World/Entity/Entity.h"

class PlayerInput : public Component
{
public:
    void OnUpdate(float delta) override;

private:
    float m_Speed = 5.0f;
    float m_JumpForce = 9.0f;
};

class Player : public Entity
{
public:
    Player();

    AABB GetAABB() const override;

    int GetRenderDistance() const { return m_RenderDistance; }

private:
    int m_RenderDistance = 8;
};
