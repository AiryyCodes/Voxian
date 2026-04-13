#pragma once

#include "World/Entity/Component/Component.h"
#include "World/Entity/Entity.h"

class PlayerInput : public Component
{
public:
    void OnUpdate(float delta) override;

private:
    float m_Speed = 4.3f;
    float m_SprintSpeed = 5.6f;
    float m_FlightSpeed = 20.0f;

    float m_JumpForce = 8.5f;

    float m_GroundAccel = 25.0f;
    float m_GroundDecel = 25.0f;
    float m_AirAccel = 6.0f;
    float m_AirDecel = 2.0f;
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
