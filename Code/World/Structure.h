#pragma once

#include "Math/Vector.h"

struct TreeBlock
{
    Vector3i RelativePos; // relative to tree base
    uint16_t BlockId;
};

struct Tree
{
    Vector3i BasePos; // world position
    std::vector<TreeBlock> Blocks;
};

struct BlockChange
{
    int X, Y, Z; // world coordinates
    uint16_t Id;
};
