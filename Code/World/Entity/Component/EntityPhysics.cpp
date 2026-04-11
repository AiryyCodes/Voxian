#include "World/Entity/Component/EntityPhysics.h"
#include "Engine.h"
#include "Logger.h"
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

    m_Velocity.y -= m_Gravity * delta;

    MoveAndCollide(Vector3f(m_Velocity.x * delta, 0, 0));
    MoveAndCollide(Vector3f(0, m_Velocity.y * delta, 0));
    MoveAndCollide(Vector3f(0, 0, m_Velocity.z * delta));

    m_Velocity.x *= m_IsOnGround ? 0.55f : 0.91f;
    m_Velocity.z *= m_IsOnGround ? 0.55f : 0.91f;
}

void EntityPhysics::MoveAndCollide(Vector3f delta)
{
    Transform &transform = GetOwner().GetComponent<Transform>();

    transform.Position += delta;
    AABB box = GetOwner().GetAABB();

    auto blocks = GetBlocksInAABB(box, delta);

    for (const auto &block : blocks)
    {
        AABB blockBox = GetBlockAABB(block);
        if (!AABB::Intersects(box, blockBox))
            continue;

        float penetration = GetPenetration(box, blockBox, delta);

        if (delta.x != 0)
        {
            transform.Position.x -= penetration;
            m_Velocity.x = 0;
        }
        else if (delta.y != 0)
        {
            transform.Position.y -= penetration;
            m_Velocity.y = 0;
        }
        else if (delta.z != 0)
        {
            transform.Position.z -= penetration;
            m_Velocity.z = 0;
        }

        box = GetOwner().GetAABB();
    }

    if (delta.y < 0 && m_Velocity.y == 0)
        m_IsOnGround = true;
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
                if (delta.y == 0 && y >= (int)floor(box.Max.y))
                    continue;

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
