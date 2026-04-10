#include "ChunkGenerator.h"
#include "World/Block/Blocks.h"
#include "World/Entity/Chunk.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"

void ChunkGenerator::Generate()
{
    Chunk &chunk = static_cast<Chunk &>(GetOwner());

    for (int x = 0; x < PADDED_CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < PADDED_CHUNK_SIZE; ++z)
        {
            for (int y = 0; y < PADDED_CHUNK_HEIGHT; ++y)
            {
                if (y > PADDED_CHUNK_HEIGHT - 64 - 2)
                {
                    chunk.SetBlock(x, y, z, Blocks::AIR);
                    continue;
                }

                if (y == PADDED_CHUNK_HEIGHT - 64 - 2)
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