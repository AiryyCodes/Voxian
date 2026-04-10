#pragma once

#include "World/Block/BlockProperties.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"

#include <array>
#include <vector>

struct ChunkSnapshot
{
    std::array<uint16_t, PADDED_CHUNK_SIZE * PADDED_CHUNK_HEIGHT * PADDED_CHUNK_SIZE> Blocks = {};
    std::vector<struct BlockProperties> BlockProperties; // indexed by blockId

    uint16_t GetBlock(int x, int y, int z) const
    {
        return Blocks[x + PADDED_CHUNK_SIZE * (y + PADDED_CHUNK_HEIGHT * z)];
    }

    const struct BlockProperties &GetBlockProperties(uint16_t blockId) const
    {
        return BlockProperties[blockId];
    }
};
