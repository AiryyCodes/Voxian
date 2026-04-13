#include "Raycast.h"
#include "Engine.h"
#include "World/Block/Block.h"

RaycastResult Raycast(const Vector3f &origin, const Vector3f &dir, float maxDist)
{
    Vector3i pos = Vector3i(std::floor(origin.x), std::floor(origin.y), std::floor(origin.z));
    Vector3i step = Vector3i(dir.x >= 0 ? 1 : -1, dir.y >= 0 ? 1 : -1, dir.z >= 0 ? 1 : -1);

    Vector3f tDelta = Vector3f(
        std::abs(1.0f / dir.x),
        std::abs(1.0f / dir.y),
        std::abs(1.0f / dir.z));

    Vector3f tMax = Vector3f(
        ((step.x > 0 ? (pos.x + 1) : pos.x) - origin.x) / dir.x,
        ((step.y > 0 ? (pos.y + 1) : pos.y) - origin.y) / dir.y,
        ((step.z > 0 ? (pos.z + 1) : pos.z) - origin.z) / dir.z);

    Vector3i normal;
    float dist = 0.0f;

    while (dist < maxDist)
    {
        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            pos.x += step.x;
            dist = tMax.x;
            tMax.x += tDelta.x;
            normal = Vector3i(-step.x, 0, 0);
        }
        else if (tMax.y < tMax.z)
        {
            pos.y += step.y;
            dist = tMax.y;
            tMax.y += tDelta.y;
            normal = Vector3i(0, -step.y, 0);
        }
        else
        {
            pos.z += step.z;
            dist = tMax.z;
            tMax.z += tDelta.z;
            normal = Vector3i(0, 0, -step.z);
        }

        const Block *block = EngineContext::GetWorld().GetChunkManager().GetBlock(pos.x, pos.y, pos.z);
        if (block && !block->GetProperties().IsAir)
            return {true, pos, normal};
    }
    return {false};
}