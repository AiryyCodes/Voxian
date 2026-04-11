#include "World/Entity/Component/EntityPhysics.h"
#include "Engine.h"
#include "Logger.h"
#include "Math/Vector.h"
#include "Physics/AABB.h"
#include "World/Block/Block.h"
#include "World/Entity/Component/Transform.h"
#include "World/World.h"
#include <glm/common.hpp>

void EntityPhysics::OnUpdate(float delta)
{
    m_Velocity.y -= m_Gravity * delta;
    Vector3f movement = m_Velocity * delta;

    Transform &transform = GetOwner().GetComponent<Transform>();
    World &world = EngineContext::GetWorld();
    m_IsOnGround = false;

    // Resolve X
    if (std::abs(movement.x) > 0)
    {
        transform.Position.x += movement.x;
        if (ResolveCollisions(world, movement.x, 0))
        {
            m_Velocity.x = 0;
        }
    }

    // Resolve Z
    if (std::abs(movement.z) > 0)
    {
        transform.Position.z += movement.z;
        if (ResolveCollisions(world, movement.z, 2))
        {
            m_Velocity.z = 0;
        }
    }

    // Resolve Y
    if (std::abs(movement.y) > 0)
    {
        transform.Position.y += movement.y;
        if (ResolveCollisions(world, movement.y, 1))
        {
            if (m_Velocity.y < 0)
                m_IsOnGround = true;
            m_Velocity.y = 0;
        }
    }
}

bool EntityPhysics::ResolveCollisions(World &world, float step, int axis)
{
    bool collided = false;
    const float EPSILON = 0.001f;

    // Get the current AABB based on the latest transform.Position
    AABB box = GetOwner().GetAABB();

    // Broadphase: Find all blocks the player's AABB overlaps
    Vector3i vMin = glm::floor(box.Min);
    Vector3i vMax = glm::floor(box.Max);

    for (int x = vMin.x; x <= vMax.x; x++)
    {
        for (int y = vMin.y; y <= vMax.y; y++)
        {
            for (int z = vMin.z; z <= vMax.z; z++)
            {

                const Block *block = world.GetBlock(x, y, z);
                if (!block || block->GetProperties().IsAir)
                    continue;

                AABB blockBox = {Vector3f(x, y, z), Vector3f(x + 1, y + 1, z + 1)};

                // Use the Intersects check
                if (AABB::Intersects(box, blockBox))
                {
                    Transform &transform = GetOwner().GetComponent<Transform>();
                    collided = true;

                    if (axis == 0)
                    { // X
                        if (step > 0)
                            transform.Position.x = blockBox.Min.x - box.GetSize().x - EPSILON;
                        else
                            transform.Position.x = blockBox.Max.x + EPSILON;
                    }
                    else if (axis == 1)
                    { // Y
                        if (step > 0)
                            transform.Position.y = blockBox.Min.y - box.GetSize().y - EPSILON;
                        else
                            transform.Position.y = blockBox.Max.y + EPSILON;
                    }
                    else if (axis == 2)
                    { // Z
                        if (step > 0)
                            transform.Position.z = blockBox.Min.z - box.GetSize().z - EPSILON;
                        else
                            transform.Position.z = blockBox.Max.z + EPSILON;
                    }

                    // Refresh box for the next block check in the loops
                    box = GetOwner().GetAABB();
                }
            }
        }
    }
    return collided;
}

float EntityPhysics::SweepAxis(int axis, float velocity, World &world)
{
    if (velocity == 0.0f)
        return 0.0f;

    AABB box = GetOwner().GetAABB();
    Vector3f expandedMin = box.Min;
    Vector3f expandedMax = box.Max;
    if (velocity < 0)
        expandedMin[axis] += velocity;
    else
        expandedMax[axis] += velocity;

    Vector3i vMin = glm::floor(expandedMin);
    Vector3i vMax = glm::floor(expandedMax);

    float allowed = velocity;

    for (int x = vMin.x; x <= vMax.x; x++)
        for (int y = vMin.y; y <= vMax.y; y++)
            for (int z = vMin.z; z <= vMax.z; z++)
            {
                const Block *block = world.GetBlock(x, y, z);
                if (!block || block->GetProperties().IsAir)
                    continue;

                float voxelMin = (axis == 0) ? x : (axis == 1) ? y
                                                               : z;

                if (velocity > 0)
                    allowed = std::min(allowed, voxelMin - box.Max[axis] - 1e-4f);
                else
                    allowed = std::max(allowed, (voxelMin + 1.0f) - box.Min[axis] + 1e-4f);
            }

    return allowed;
}

float EntityPhysics::SweepAxisY(float velocity, World &world)
{
    AABB box = GetOwner().GetAABB();

    const float xzShrink = 0.05f; // ← shrink inward so barely-touched walls are excluded

    Vector3f expandedMin = box.Min;
    Vector3f expandedMax = box.Max;
    if (velocity < 0)
        expandedMin.y += velocity;
    else
        expandedMax.y += velocity;

    Vector3i vMin = glm::floor(expandedMin);
    Vector3i vMax = glm::floor(expandedMax);

    float allowed = velocity;

    for (int x = vMin.x; x <= vMax.x; x++)
        for (int y = vMin.y; y <= vMax.y; y++)
            for (int z = vMin.z; z <= vMax.z; z++)
            {
                const Block *block = world.GetBlock(x, y, z);
                if (!block || block->GetProperties().IsAir)
                    continue;

                // Use a shrunk XZ footprint — ignore blocks only barely overlapping
                if (box.Max.x - xzShrink <= x || box.Min.x + xzShrink >= x + 1.0f)
                    continue;
                if (box.Max.z - xzShrink <= z || box.Min.z + xzShrink >= z + 1.0f)
                    continue;

                if (velocity < 0)
                    allowed = std::max(allowed, (float)(y + 1) - box.Min.y + 1e-4f);
                else
                    allowed = std::min(allowed, (float)y - box.Max.y - 1e-4f);
            }
    return allowed;
}
