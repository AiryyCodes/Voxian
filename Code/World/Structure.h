#pragma once

#include "Math/Vector.h"

struct TreeBlock
{
    // Relative to tree base
    Vector3i RelativePos;
    uint16_t BlockId;
};

struct Tree
{
    // World position
    Vector3i BasePos;
    std::vector<TreeBlock> Blocks;
};
