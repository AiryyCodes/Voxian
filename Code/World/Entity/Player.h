#pragma once

#include "World/Entity/Component/Component.h"
#include "World/Entity/Entity.h"

class PlayerInput : public Component
{
public:
    void OnUpdate(float delta) override;

private:
    float m_Speed = 4.3f;
    float m_SprintSpeed = 50.6f;

    float m_JumpForce = 9.0f;
};

class SpawnController : public Component
{
public:
    void OnUpdate(float delta) override;
};

class Player : public Entity
{
public:
    Player();

    void SetPhysicsEnabled(bool enabled);

    AABB GetAABB() const override;

    int GetRenderDistance() const { return m_RenderDistance; }

private:
    int m_RenderDistance = 8;
};
