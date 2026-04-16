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

    constexpr int MAX_BIOME_WEIGHTS = 3;
    struct WeightEntry
    {
        const BiomeConfig *Biome;
        float Weight;
    };

    struct ColumnWeights
    {
        WeightEntry Entries[MAX_BIOME_WEIGHTS];
        float NoiseValues[MAX_BIOME_WEIGHTS]; // ADD THIS
        int Count = 0;
    };

    std::vector<ColumnWeights> biomeGrid((CX * CELL + 1) * (CZ * CELL + 1));
    auto biomeAt = [&](int x, int z) -> ColumnWeights &
    {
        return biomeGrid[x * (CZ * CELL + 1) + z];
    };

    // Stack buffer — no heap allocation per column
    BiomeWeight weightsBuf[MAX_BIOME_WEIGHTS];

    std::vector<float> densityCache;

    for (int x = 0; x <= CX * CELL; x++)
    {
        for (int z = 0; z <= CZ * CELL; z++)
        {
            float wx = origin.x + x;
            float wz = origin.z + z;
            int count = noise.SampleBiomeWeights(wx, wz, weightsBuf, MAX_BIOME_WEIGHTS);
            auto &col = biomeAt(x, z);
            col.Count = count;
            for (int i = 0; i < count; i++)
            {
                col.Entries[i] = {weightsBuf[i].Biome, weightsBuf[i].Weight};
                col.NoiseValues[i] = noise.GetNoise(wx, wz, weightsBuf[i].Biome);
            }
        }
    }

    std::vector<float> densityFlat((CX + 1) * (CY + 1) * (CZ + 1));
    auto density = [&](int x, int y, int z) -> float &
    {
        return densityFlat[x * (CY + 1) * (CZ + 1) + y * (CZ + 1) + z];
    };

    for (int cx = 0; cx <= CX; cx++)
    {
        for (int cy = 0; cy <= CY; cy++)
        {
            for (int cz = 0; cz <= CZ; cz++)
            {
                float wx = origin.x + cx * CELL;
                float wy = origin.y + cy * CELL;
                float wz = origin.z + cz * CELL;

                auto &col = biomeAt(cx * CELL, cz * CELL);

                float blendedOffset = 0.0f, blendedScale = 0.0f;
                for (int i = 0; i < col.Count; i++)
                {
                    float n = col.NoiseValues[i];
                    blendedOffset += noise.ApplyOffset(n, col.Entries[i].Biome) * col.Entries[i].Weight;
                    blendedScale += noise.ApplyScale(n, col.Entries[i].Biome) * col.Entries[i].Weight;
                }

                float yFrac = (wy - config.HeightRange.Min) /
                              (float)(config.HeightRange.Max - config.HeightRange.Min);
                density(cx, cy, cz) = (blendedOffset - yFrac) / std::max(blendedScale, 0.001f);
            }
        }
    }

    for (int x = 0; x < PADDED_CHUNK_SIZE; x++)
    {
        for (int y = 0; y < PADDED_CHUNK_HEIGHT; y++)
        {
            for (int z = 0; z < PADDED_CHUNK_SIZE; z++)
            {
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

                bool isAtSurface = false;

                for (int i = 1; i <= 3; i++)
                {
                    float above = Trilinear(densityFlat,
                                            cx,
                                            cy + i,
                                            cz,
                                            tx,
                                            0,
                                            tz);

                    if (above <= 0.0f)
                    {
                        isAtSurface = true;
                        break;
                    }
                }

                int depthFromSurface = 0;

                if (!isAtSurface)
                {
                    bool foundAir = false;

                    for (int scanY = y + 1;
                         scanY < PADDED_CHUNK_HEIGHT && depthFromSurface <= biomeRegistry.GetMaxScanDepth();
                         scanY++)
                    {
                        float sd = Trilinear(
                            densityFlat,
                            x / CELL,
                            scanY / CELL,
                            z / CELL,
                            (x % CELL) / (float)CELL,
                            (scanY % CELL) / (float)CELL,
                            (z % CELL) / (float)CELL);

                        if (sd <= 0.0f)
                        {
                            foundAir = true;
                            break;
                        }

                        depthFromSurface++;
                    }

                    if (!foundAir)
                    {
                        depthFromSurface = biomeRegistry.GetMaxScanDepth() + 1;
                    }
                }

                const BiomeConfig *biome = biomeAt(x, z).Entries[0].Biome;
                float wy = origin.y + y;
                chunk.m_Blocks.SetId(x, y, z, ResolveBlock((int)wy, isAtSurface, depthFromSurface, config.SeaLevel, *biome));
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

float ChunkGenerator::Trilinear(const std::vector<float> &d, int cx, int cy, int cz, float tx, float ty, float tz)
{
    const int strideZ = CZ + 1;
    const int strideY = CY + 1;
    const int layer = strideY * strideZ;

    const int base = cx * layer + cy * strideZ + cz;

    const float *p = d.data();

    const float c000 = p[base];
    const float c100 = p[base + layer];
    const float c010 = p[base + strideZ];
    const float c110 = p[base + layer + strideZ];

    const float c001 = p[base + 1];
    const float c101 = p[base + layer + 1];
    const float c011 = p[base + strideZ + 1];
    const float c111 = p[base + layer + strideZ + 1];

    const float tx1 = 1.0f - tx;
    const float ty1 = 1.0f - ty;
    const float tz1 = 1.0f - tz;

    const float d00 = c000 * tx1 + c100 * tx;
    const float d01 = c001 * tx1 + c101 * tx;
    const float d10 = c010 * tx1 + c110 * tx;
    const float d11 = c011 * tx1 + c111 * tx;

    const float dz0 = d00 * tz1 + d01 * tz;
    const float dz1 = d10 * tz1 + d11 * tz;

    return dz0 * ty1 + dz1 * ty;
}