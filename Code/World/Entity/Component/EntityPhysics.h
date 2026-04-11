#pragma once

#include "Math/Vector.h"
#include "World/Entity/Component/Component.h"
#include "World/World.h"

class EntityPhysics : public Component
{
public:
    void OnUpdate(float delta) override;

private:
    bool ResolveCollisions(World &world, float step, int axis);

    float SweepAxis(int axis, float velocity, World &world);
    float SweepAxisY(float velocity, World &world);

private:
    float m_Gravity = 28.0f;
    Vector3f m_Velocity;
    bool m_IsOnGround;
};
