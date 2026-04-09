#pragma once

#include "Math/Vector.h"
#include "World/Entity/Entity.h"

#include <cstdint>
#include <vector>

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

struct ChunkBlockData
{
    static constexpr int PADDED_SIZE = CHUNK_SIZE + 2;
    static constexpr int PADDED_HEIGHT = CHUNK_HEIGHT + 2;

    std::vector<uint16_t> Indices;

    ChunkBlockData()
    {
        Indices.resize(PADDED_SIZE * PADDED_HEIGHT * PADDED_SIZE);
    }

    uint32_t GetIndex(int x, int y, int z) const
    {
        return x + PADDED_SIZE * (y + PADDED_HEIGHT * z);
    }

    uint16_t GetId(int x, int y, int z) const
    {
        uint32_t index = GetIndex(x, y, z);
        if (index < Indices.size())
            return Indices[index];
        return 0; // Default to air if out of bounds
    }

    void SetId(int x, int y, int z, uint16_t id)
    {
        uint32_t index = GetIndex(x, y, z);
        if (index < Indices.size())
            Indices[index] = id;
    }
};

class Chunk : public Entity
{
public:
    Chunk(Vector2i position);
    ~Chunk();

    uint16_t GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, uint16_t id);

private:
    Vector2i m_Position;

    ChunkBlockData m_Blocks;
};