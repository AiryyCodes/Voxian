#include "World/Entity/Component/EntityPhysics.h"
#include "Engine.h"
#include "Math/Vector.h"
#include "Physics/AABB.h"
#include "World/Block/Block.h"
#include "World/Entity/Component/Transform.h"
#include "World/World.h"
#include <cmath>
#include <glm/common.hpp>
#include <vector>

void EntityPhysics::OnUpdate(float delta)
{
    m_IsOnGround = false;
    if (!m_IsFlying)
        m_Velocity.y -= m_Gravity * delta;

    Vector3f motion(m_Velocity.x * delta, m_Velocity.y * delta, m_Velocity.z * delta);

    // Resolve in passes until clean or max iterations reached
    constexpr int MAX_ITERATIONS = 3;
    for (int i = 0; i < MAX_ITERATIONS; i++)
    {
        bool anyCollision = false;
        anyCollision |= MoveAndCollide(Vector3f(motion.x, 0, 0));
        anyCollision |= MoveAndCollide(Vector3f(0, motion.y, 0));
        anyCollision |= MoveAndCollide(Vector3f(0, 0, motion.z));

        // Zero out resolved axes for subsequent iterations
        if (m_Velocity.x == 0)
            motion.x = 0;
        if (m_Velocity.y == 0)
            motion.y = 0;
        if (m_Velocity.z == 0)
            motion.z = 0;

        if (!anyCollision)
            break;
    }
}

bool EntityPhysics::MoveAndCollide(Vector3f delta)
{
    if (delta.x == 0 && delta.y == 0 && delta.z == 0)
        return false;

    Transform &transform = GetOwner().GetComponent<Transform>();
    transform.Position += delta;
    AABB box = GetOwner().GetAABB();
    auto blocks = GetBlocksInAABB(box, delta);

    std::sort(blocks.begin(), blocks.end(), [&](const Vector3i &a, const Vector3i &b)
              {
        float pa = std::abs(GetPenetration(box, GetBlockAABB(a), delta));
        float pb = std::abs(GetPenetration(box, GetBlockAABB(b), delta));
        return pa < pb; });

    bool resolved = false;
    for (const auto &block : blocks)
    {
        box = GetOwner().GetAABB();
        AABB blockBox = GetBlockAABB(block);
        if (!AABB::Intersects(box, blockBox))
            continue;

        float correction = GetPenetration(box, blockBox, delta);

        if (delta.x != 0)
        {
            transform.Position.x -= correction;
            m_Velocity.x = 0;
            resolved = true;
        }
        else if (delta.y != 0)
        {
            transform.Position.y -= correction;
            m_Velocity.y = 0;
            resolved = true;
        }
        else if (delta.z != 0)
        {
            transform.Position.z -= correction;
            m_Velocity.z = 0;
            resolved = true;
        }

        box = GetOwner().GetAABB();
    }

    if (!m_IsFlying && delta.y < 0 && m_Velocity.y == 0)
        m_IsOnGround = true;

    return resolved;
}

std::vector<Vector3i> EntityPhysics::GetBlocksInAABB(const AABB &box, Vector3f delta)
{
    World &world = EngineContext::GetWorld();

    std::vector<Vector3i> blocks;

    for (int x = floor(box.Min.x); x <= floor(box.Max.x); x++)
    {
        for (int y = floor(box.Min.y); y <= floor(box.Max.y); y++)
        {
            for (int z = floor(box.Min.z); z <= floor(box.Max.z); z++)
            {
                const Block *block = world.GetBlock(x, y, z);
                if (!block || block->GetProperties().IsAir)
                    continue;

                blocks.push_back(Vector3i(x, y, z));
            }
        }
    }

    return blocks;
}

AABB EntityPhysics::GetBlockAABB(Vector3i blockPos)
{
    return {
        Vector3f(blockPos),
        Vector3f(blockPos) + 1.0f,
    };
}

float EntityPhysics::GetPenetration(const AABB &entity, const AABB &block, const Vector3f &delta)
{
    if (delta.x != 0)
    {
        if (delta.x > 0)
            return entity.Max.x - block.Min.x; // hit left face of block
        else
            return entity.Min.x - block.Max.x; // hit right face (negative)
    }
    if (delta.y != 0)
    {
        if (delta.y > 0)
            return entity.Max.y - block.Min.y; // hit bottom face
        else
            return entity.Min.y - block.Max.y; // hit top face (landing)
    }
    if (delta.z != 0)
    {
        if (delta.z > 0)
            return entity.Max.z - block.Min.z;
        else
            return entity.Min.z - block.Max.z;
    }
    return 0;
}
