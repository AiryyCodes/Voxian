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
        int Count = 0;
    };

    std::vector<ColumnWeights> biomeGrid((CX * CELL + 1) * (CZ * CELL + 1));
    auto biomeAt = [&](int x, int z) -> ColumnWeights &
    {
        return biomeGrid[x * (CZ * CELL + 1) + z];
    };

    // Stack buffer — no heap allocation per column
    BiomeWeight weightsBuf[MAX_BIOME_WEIGHTS];

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
                col.Entries[i] = {weightsBuf[i].Biome, weightsBuf[i].Weight};
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
                    float n = noise.GetNoise(wx, wz, col.Entries[i].Biome);
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

                bool isAtSurface = dAbove <= 0.0f;
                int depthFromSurface = 0;

                if (!isAtSurface)
                {
                    bool foundAir = false;
                    for (int scanY = y + 1;
                         scanY < PADDED_CHUNK_HEIGHT && depthFromSurface <= biomeRegistry.GetMaxScanDepth();
                         scanY++)
                    {
                        int scy = scanY / CELL;
                        float sty = (scanY % CELL) / (float)CELL;
                        float sd = Trilinear(densityFlat, cx, scy, cz, tx, sty, tz);
                        if (sd <= 0.0f)
                        {
                            foundAir = true;
                            break;
                        }
                        depthFromSurface++;
                    }
                    // Deep underground — exceeds all MaxDepth layers, stone will match
                    if (!foundAir)
                        depthFromSurface = biomeRegistry.GetMaxScanDepth() + 1;
                }

                const BiomeConfig *biome = biomeAt(x, z).Entries[0].Biome;
                float wy = origin.y + y;
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
    int strideY = CY + 1;
    int strideZ = CZ + 1;
    auto s = [&](int x, int y, int z) -> float
    {
        return d[x * strideY * strideZ + y * strideZ + z];
    };
    float d00 = s(cx, cy, cz) * (1 - tx) + s(cx + 1, cy, cz) * tx;
    float d01 = s(cx, cy, cz + 1) * (1 - tx) + s(cx + 1, cy, cz + 1) * tx;
    float d10 = s(cx, cy + 1, cz) * (1 - tx) + s(cx + 1, cy + 1, cz) * tx;
    float d11 = s(cx, cy + 1, cz + 1) * (1 - tx) + s(cx + 1, cy + 1, cz + 1) * tx;
    float d0 = d00 * (1 - tz) + d01 * tz;
    float d1 = d10 * (1 - tz) + d11 * tz;
    return d0 * (1 - ty) + d1 * ty;
}