#pragma once

#include <vector>
#include <cstdint>

struct ChunkVertex
{
    uint32_t Data1; // x:6, y:9, z:6, normalIndex:3 = 24 bits used
    uint32_t Data2; // cornerIndex:2, textureIndex:16 = 18 bits used
};

struct ChunkMeshData
{
    std::vector<ChunkVertex> Vertices;
    std::vector<unsigned int> Indices;
};

inline ChunkVertex MakeVertex(
    int x, int y, int z,
    int normalIndex, // 0-5
    int cornerIndex, // 0-3
    int textureIndex)
{
    ChunkVertex v;
    v.Data1 = (x & 0x3F) | ((y & 0x1FF) << 6) | ((z & 0x3F) << 15) | ((normalIndex & 0x7) << 21);
    v.Data2 = (cornerIndex & 0x3) | ((textureIndex & 0xFFFF) << 2);
    return v;
}
