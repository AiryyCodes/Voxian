#pragma once

#include "Math/Vector.h"

struct RaycastResult
{
    bool Hit;
    Vector3i BlockPos;
    Vector3i Normal;
};

RaycastResult Raycast(const Vector3f &origin, const Vector3f &dir, float maxDist);