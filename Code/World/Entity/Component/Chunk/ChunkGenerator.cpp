#include "ChunkGenerator.h"
#include "Engine.h"
#include "Math/Vector.h"
#include "World/Block/Blocks.h"
#include "World/Chunk/Terrain/Biome/BiomeConfig.h"
#include "World/Chunk/Terrain/Biome/BiomeRegistry.h"
#include "World/Chunk/Terrain/TerrainNoise.h"
#include "World/Entity/Chunk.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include <cstdint>

void ChunkGenerator::Generate(const TerrainNoise &noise)
{
    Chunk &chunk = static_cast<Chunk &>(GetOwner());
    Vector3i origin = chunk.GetWorldPosition() - Vector3i(1, 1, 1);
    const TerrainConfig &config = noise.GetConfig();
    BiomeRegistry &biomeRegistry = EngineContext::GetBiomeRegistry();

    std::vector<float> densityFlat(CX * CY * CZ);
    auto density = [&](int x, int y, int z) -> float &
    {
        return densityFlat[x * CY * CZ + y * CZ + z];
    };

    // Fill density grid
    for (int cx = 0; cx < CX; cx++)
    {
        for (int cy = 0; cy < CY; cy++)
        {
            for (int cz = 0; cz < CZ; cz++)
            {
                float wx = origin.x + cx * CELL;
                float wy = origin.y + cy * CELL;
                float wz = origin.z + cz * CELL;
                density(cx, cy, cz) = noise.GetDensity(wx, wy, wz);
            }
        }
    }

    // Fill blocks
    for (int x = 0; x < PADDED_CHUNK_SIZE; x++)
    {
        for (int y = 0; y < PADDED_CHUNK_HEIGHT; y++)
        {
            for (int z = 0; z < PADDED_CHUNK_SIZE; z++)
            {
                float wx = origin.x + x;
                float wy = origin.y + y;
                float wz = origin.z + z;

                int cx = x / CELL, cy = y / CELL, cz = z / CELL;
                float tx = (x % CELL) / (float)CELL;
                float ty = (y % CELL) / (float)CELL;
                float tz = (z % CELL) / (float)CELL;

                float d = Trilinear(densityFlat, cx, cy, cz, tx, ty, tz);
                float dAbove = Trilinear(densityFlat, cx, cy + 1, cz, tx, 0, tz);

                if (d <= 0.0f)
                {
                    chunk.m_Blocks.SetId(x, y, z, Blocks::AIR);
                    continue;
                }

                bool isAtSurface = dAbove <= 0.0f;
                int depthFromSurface = 0;
                if (!isAtSurface)
                {
                    for (int scanY = y + 1; scanY < PADDED_CHUNK_HEIGHT && depthFromSurface <= biomeRegistry.GetMaxScanDepth(); scanY++)
                    {
                        int scx = x / CELL, scz = z / CELL;
                        int scy = scanY / CELL;
                        float stx = (x % CELL) / (float)CELL;
                        float sty = (scanY % CELL) / (float)CELL;
                        float stz = (z % CELL) / (float)CELL;
                        float sd = Trilinear(densityFlat, scx, scy, scz, stx, sty, stz);
                        if (sd <= 0.0f)
                            break; // found air — depthFromSurface is the gap so far
                        depthFromSurface++;
                    }
                }

                const BiomeConfig *biome = noise.SelectBiome(wx, wz);
                chunk.m_Blocks.SetId(x, y, z,
                                     ResolveBlock((int)wy, isAtSurface, depthFromSurface, config.SeaLevel, *biome));
            }
        }
    }
}

uint16_t ChunkGenerator::ResolveBlock(int worldY, bool isAtSurface, int depthFromSurface, int seaLevel, const BiomeConfig &biome) const
{
    for (auto &layer : biome.BlockLayers)
    {
        if (layer.MinHeight != -1 && worldY < layer.MinHeight)
            continue;
        if (layer.MaxHeight != -1 && worldY > layer.MaxHeight)
            continue;
        if (layer.RequiresBelowSeaLevel && worldY > seaLevel)
            continue;

        switch (layer.Condition)
        {
        case LayerCondition::AtBottom:
            if (worldY == 0)
                return layer.BlockIndex;
            break;

        case LayerCondition::AtSurface:
            if (isAtSurface)
                return layer.BlockIndex;
            break;

        case LayerCondition::BelowSurface:
            if (!isAtSurface && depthFromSurface > 0)
                if (layer.MaxDepth == -1 || depthFromSurface <= layer.MaxDepth)
                    return layer.BlockIndex;
            break;

        case LayerCondition::BelowSeaLevel:
            if (worldY < seaLevel)
                return layer.BlockIndex;
            break;
        }
    }
    return Blocks::AIR;
}

float ChunkGenerator::Trilinear(const std::vector<float> &d,
                                int cx, int cy, int cz, float tx, float ty, float tz)
{
    auto s = [&](int x, int y, int z) -> float
    {
        x = std::min(x, CX - 1);
        y = std::min(y, CY - 1);
        z = std::min(z, CZ - 1);
        return d[x * CY * CZ + y * CZ + z];
    };
    float d00 = s(cx, cy, cz) * (1 - tx) + s(cx + 1, cy, cz) * tx;
    float d01 = s(cx, cy, cz + 1) * (1 - tx) + s(cx + 1, cy, cz + 1) * tx;
    float d10 = s(cx, cy + 1, cz) * (1 - tx) + s(cx + 1, cy + 1, cz) * tx;
    float d11 = s(cx, cy + 1, cz + 1) * (1 - tx) + s(cx + 1, cy + 1, cz + 1) * tx;
    float d0 = d00 * (1 - tz) + d01 * tz;
    float d1 = d10 * (1 - tz) + d11 * tz;
    return d0 * (1 - ty) + d1 * ty;
}