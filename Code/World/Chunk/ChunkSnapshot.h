#pragma once

#include "World/Block/BlockProperties.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"

#include <cstdint>
#include <vector>

struct ChunkSnapshot
{
    std::vector<uint16_t> Blocks = {};
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
