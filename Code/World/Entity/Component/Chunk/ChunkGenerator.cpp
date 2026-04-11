#include "ChunkGenerator.h"
#include "Math/Vector.h"
#include "Math/Math.h"
#include "World/Block/Blocks.h"
#include "World/Chunk/Terrain/TerrainNoise.h"
#include "World/Entity/Chunk.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include <cstdint>

void ChunkGenerator::Generate(const TerrainNoise &noise)
{
    Chunk &chunk = static_cast<Chunk &>(GetOwner());

    Vector3i origin = chunk.GetWorldPosition();

    const TerrainConfig &config = noise.GetConfig();

    for (int x = 0; x < PADDED_CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < PADDED_CHUNK_SIZE; ++z)
        {
            float noiseVal = noise.GetNoise(
                (float)(origin.x + x),
                (float)(origin.z + z));

            int surfaceY = static_cast<int>(Math::Remap(
                noiseVal, -1.0f, 1.0f,
                0.0f,
                static_cast<float>(PADDED_CHUNK_HEIGHT)));

            for (int y = 0; y < PADDED_CHUNK_HEIGHT; ++y)
            {
                int worldY = origin.y + y;
                uint16_t resolvedBlock = ResolveBlock(config, worldY, surfaceY);
                chunk.m_Blocks.SetId(x, y, z, resolvedBlock);
            }
        }
    }
}

uint16_t ChunkGenerator::ResolveBlock(const TerrainConfig &config, int worldY, int surfaceY) const
{
    const int depth = surfaceY - worldY;
    const bool belowSea = worldY < config.SeaLevel;
    const bool isBottom = worldY == 0;
    const bool isAbove = worldY > surfaceY;

    for (const auto &layer : config.Layers)
    {
        if (layer.RequiresBelowSeaLevel && !belowSea)
            continue;

        switch (layer.Condition)
        {
        case LayerCondition::AtBottom:
            if (!isBottom)
                continue;
            break;
        case LayerCondition::BelowSeaLevel:
            if (!isAbove || !belowSea)
                continue;
            break;
        case LayerCondition::AtSurface:
            if (depth != 0)
                continue;
            break;
        case LayerCondition::BelowSurface:
            if (depth <= 0)
                continue;
            if (layer.MaxDepth >= 0 && depth > layer.MaxDepth)
                continue;
            break;
        }

        return layer.BlockIndex;
    }

    return Blocks::AIR;
}
