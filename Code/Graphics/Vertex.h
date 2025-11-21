#pragma once

#include "Math/Vector.h"

struct BlockVertex
{
    Vector3f Position;
    Vector3f Normal;
    Vector2f UV;
    Vector2i TextureSize;
    int Layer;
    float AO;
};
