#pragma once

#include "World/Entity/Component/Component.h"

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

#define PADDED_CHUNK_SIZE (CHUNK_SIZE + 2)
#define PADDED_CHUNK_HEIGHT (CHUNK_HEIGHT + 2)

class ChunkGenerator : public Component
{
public:
    void Generate();
};