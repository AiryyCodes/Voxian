#pragma once

#include "Math/Vector.h"

struct AABB
{
    Vector3f Min;
    Vector3f Max;

    bool IsValid() const;

    Vector3f GetSize() const
    {
        return Max - Min;
    }

    // Static helper for intersection testing
    static bool Intersects(const AABB &a, const AABB &b)
    {
        return (a.Min.x < b.Max.x && a.Max.x > b.Min.x) &&
               (a.Min.y < b.Max.y && a.Max.y > b.Min.y) &&
               (a.Min.z < b.Max.z && a.Max.z > b.Min.z);
    }
};
