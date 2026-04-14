#pragma once

#include "World/Entity/Component/Component.h"
#include "World/Entity/Entity.h"

#include <cstdint>

class PlayerInput : public Component
{
public:
    void OnUpdate(float delta) override;

private:
    float m_Speed = 3.3f;
    float m_SprintSpeed = 4.6f;
    float m_FlightSpeed = 20.0f;

    float m_JumpForce = 8.5f;

    float m_GroundAccel = 25.0f;
    float m_GroundDecel = 25.0f;
    float m_AirAccel = 6.0f;
    float m_AirDecel = 2.0f;

    uint16_t m_SelectedBlock = 1;
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
