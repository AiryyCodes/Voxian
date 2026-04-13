#pragma once

#include "Math/Vector.h"
#include "Physics/AABB.h"
#include "World/Entity/Component/Component.h"

#include <vector>

class EntityPhysics : public Component
{
public:
    void OnUpdate(float delta) override;

    bool IsEnabled() const { return m_IsEnabled; }
    void SetEnabled(bool enabled) { m_IsEnabled = enabled; }

    Vector3f GetVelocity() const { return m_Velocity; }
    void AddVelocity(Vector3f v) { m_Velocity += v; }
    void SetVelocity(Vector3f v) { m_Velocity = v; }

    bool IsOnGround() const { return m_IsOnGround; }

    bool IsFlying() const { return m_IsFlying; }
    void SetFlying(bool flying) { m_IsFlying = flying; }

private:
    bool MoveAndCollide(Vector3f delta);
    std::vector<Vector3i> GetBlocksInAABB(const AABB &box, Vector3f delta);
    AABB GetBlockAABB(Vector3i blockPos);
    float GetPenetration(const AABB &entity, const AABB &block, const Vector3f &delta);

private:
    bool m_IsEnabled = true;

    float m_Gravity = 28.0f;
    Vector3f m_Velocity;
    bool m_IsOnGround;

    bool m_IsFlying = false;
};
