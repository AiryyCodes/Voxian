#pragma once

#include "Math/Vector.h"

#include <vector>

struct ChunkVertex
{
    Vector3f Position;
    Vector3f Normal;
    Vector2f UV;
    int TextureIndex;
};

struct ChunkMeshData
{
    std::vector<ChunkVertex> Vertices;
    std::vector<unsigned int> Indices;
};