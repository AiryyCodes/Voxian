#include "ChunkGenerator.h"
#include "World/Entity/Chunk.h"
#include "World/Block/Blocks.h"

void ChunkGenerator::Generate()
{
    Chunk &chunk = static_cast<Chunk &>(GetOwner());

    for (int x = 0; x < ChunkBlockData::PADDED_SIZE; ++x)
    {
        for (int z = 0; z < ChunkBlockData::PADDED_SIZE; ++z)
        {
            for (int y = 0; y < ChunkBlockData::PADDED_HEIGHT; ++y)
            {
                if (y == ChunkBlockData::PADDED_HEIGHT - 3)
                {
                    chunk.SetBlock(x, y, z, Blocks::GRASS_BLOCK);
                }
                else
                {
                    chunk.SetBlock(x, y, z, Blocks::STONE); // Fill the rest with stone for testing
                }
            }
        }
    }
}