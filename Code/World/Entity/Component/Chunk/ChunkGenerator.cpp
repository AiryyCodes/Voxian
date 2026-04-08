#include "ChunkGenerator.h"
#include "World/Entity/Chunk.h"

void ChunkGenerator::Generate()
{
    Chunk &chunk = static_cast<Chunk &>(GetOwner());

    for (int x = 0; x < CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < CHUNK_SIZE; ++z)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                chunk.SetBlock(x, y, z, true); // Placeholder: make every block solid for now
            }
        }
    }
}