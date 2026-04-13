#pragma once

#include "World/Chunk/Terrain/TerrainNoise.h"
#include "World/Entity/Component/Component.h"

#include <cstdint>

#define CHUNK_SIZE 32
#define CHUNK_HEIGHT 384

#define PADDED_CHUNK_SIZE (CHUNK_SIZE + 2)
#define PADDED_CHUNK_HEIGHT (CHUNK_HEIGHT + 2)

#define CELL 2
#define CX ((PADDED_CHUNK_SIZE / CELL) + 2)
#define CY ((PADDED_CHUNK_HEIGHT / CELL) + 2)
#define CZ ((PADDED_CHUNK_SIZE / CELL) + 2)

class ChunkGenerator : public Component
{
public:
    void Generate(const TerrainNoise &noise);

private:
    uint16_t ResolveBlock(int worldY, bool isAtSurface, int depthFromSurface, int seaLevel, const BiomeConfig &biome) const;

    float Trilinear(const std::vector<float> &d,
                    int cx, int cy, int cz, float tx, float ty, float tz);
};