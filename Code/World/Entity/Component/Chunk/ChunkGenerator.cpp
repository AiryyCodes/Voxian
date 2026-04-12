#include "ChunkGenerator.h"
#include "Math/Vector.h"
#include "World/Block/Blocks.h"
#include "World/Chunk/Terrain/Biome/BiomeConfig.h"
#include "World/Chunk/Terrain/TerrainNoise.h"
#include "World/Entity/Chunk.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include <cstdint>

void ChunkGenerator::Generate(const TerrainNoise &noise)
{
    Chunk &chunk = static_cast<Chunk &>(GetOwner());

    // Chunk blocks are stored with a 1-block padding on every side.
    // Subtracting (1,1,1) lets x/z border cells sample the neighboring world coordinates.
    Vector3i origin = chunk.GetWorldPosition() - Vector3i(1, 1, 1);

    const TerrainConfig &config = noise.GetConfig();

    for (int x = 0; x < PADDED_CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < PADDED_CHUNK_SIZE; ++z)
        {
            int worldX_int = origin.x + x;
            int worldZ_int = origin.z + z;
            float worldX = static_cast<float>(worldX_int);
            float worldZ = static_cast<float>(worldZ_int);

            const BiomeConfig *biome = noise.SelectBiome(worldX, worldZ);
            float raw = noise.GetNoise(worldX, worldZ, biome);
            float curved = noise.ApplyCurve(raw, biome);
            int surfaceY = (int)(curved * (config.HeightRange.Max - config.HeightRange.Min) + config.HeightRange.Min);

            for (int y = 0; y < PADDED_CHUNK_HEIGHT; ++y)
            {
                int worldY = origin.y + y;
                if (worldY < 0)
                {
                    chunk.m_Blocks.SetId(x, y, z, Blocks::AIR);
                    continue;
                }

                uint16_t resolvedBlock = ResolveBlock(worldY, surfaceY, config.SeaLevel, *biome);
                chunk.m_Blocks.SetId(x, y, z, resolvedBlock);
            }
        }
    }
}

uint16_t ChunkGenerator::ResolveBlock(int y, int surfaceY, int seaLevel, const BiomeConfig &biome) const
{
    int depthFromSurface = surfaceY - y;

    for (auto &layer : biome.BlockLayers) // already sorted by Priority
    {
        // height bounds check
        if (layer.MinHeight != -1 && y < layer.MinHeight)
            continue;
        if (layer.MaxHeight != -1 && y > layer.MaxHeight)
            continue;

        // sea level check
        if (layer.RequiresBelowSeaLevel && y > seaLevel)
            continue;

        switch (layer.Condition)
        {
        case LayerCondition::AtBottom:
            if (y == 0)
                return layer.BlockIndex;
            break;

        case LayerCondition::AtSurface:
            if (y == surfaceY)
                return layer.BlockIndex;
            break;

        case LayerCondition::BelowSurface:
            if (depthFromSurface > 0)
                if (layer.MaxDepth == -1 || depthFromSurface <= layer.MaxDepth)
                    return layer.BlockIndex;
            break;

        case LayerCondition::BelowSeaLevel:
            if (y < seaLevel && y < surfaceY)
                return layer.BlockIndex;
            break;
        }
    }

    return Blocks::AIR; // above surface
}
