#pragma once

#include "World/Chunk/Terrain/TerrainNoise.h"
#include "World/Entity/Component/Component.h"

#include <cstdint>

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

#define PADDED_CHUNK_SIZE (CHUNK_SIZE + 2)
#define PADDED_CHUNK_HEIGHT (CHUNK_HEIGHT + 2)

class ChunkGenerator : public Component
{
public:
    void Generate(const TerrainNoise &noise);

private:
    uint16_t ResolveBlock(const TerrainConfig &config, int worldY, int surfaceY) const;
};