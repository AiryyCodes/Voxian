#include "ChunkGenerator.h"
#include "World/Entity/Chunk.h"
#include "World/Block/Blocks.h"

void ChunkGenerator::Generate()
{
    Chunk &chunk = static_cast<Chunk &>(GetOwner());

    for (int x = 0; x < BlockData::PADDED_SIZE; ++x)
    {
        for (int z = 0; z < BlockData::PADDED_SIZE; ++z)
        {
            for (int y = 0; y < BlockData::PADDED_HEIGHT; ++y)
            {
                if (y == BlockData::PADDED_HEIGHT - 4)
                    chunk.SetBlock(x, y, z, Blocks::AIR); // Top layer is air
                else
                    chunk.SetBlock(x, y, z, Blocks::GRASS_BLOCK); // Placeholder: make every block a grass block for testing
            }
        }
    }
}