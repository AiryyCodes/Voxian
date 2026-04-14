#pragma once

#include "Math/Vector.h"

#include <vector>
#include <cstdint>

struct ChunkVertex
{
    uint32_t Data1;
    uint32_t Data2;
    Vector4f UVBounds;
};

struct ChunkMeshData
{
    std::vector<ChunkVertex> Vertices;
    std::vector<unsigned int> Indices;
};

struct ChunkMeshGroup
{
    ChunkMeshData Opaque;
    ChunkMeshData Transparent;
};

inline ChunkVertex MakeVertex(
    float x, float y, float z,
    int normalIndex, // 0-5
    int cornerIndex, // 0-3
    int textureIndex,
    float ao,
    const Vector4f &uvBounds)
{
    auto encodeCoord = [](float value, uint32_t maxValue)
    {
        int encoded = static_cast<int>(value * 16.0f);
        if (encoded < 0)
            encoded = 0;
        if (encoded > static_cast<int>(maxValue))
            encoded = static_cast<int>(maxValue);
        return static_cast<uint32_t>(encoded);
    };

    uint32_t xi = encodeCoord(x, 0x3FFu);
    uint32_t yi = encodeCoord(y, 0xFFFu);
    uint32_t zi = encodeCoord(z, 0x3FFu);

    ChunkVertex v;
    v.Data1 = xi | (yi << 10) | (zi << 22);

    uint32_t aoInt = static_cast<uint32_t>(ao * 3.0f + 0.5f);
    v.Data2 = (cornerIndex & 0x3u) | ((textureIndex & 0xFFFFu) << 2) | ((static_cast<uint32_t>(normalIndex) & 0x7u) << 18) | ((aoInt & 0x3u) << 21);
    v.UVBounds = uvBounds;
    return v;
}
