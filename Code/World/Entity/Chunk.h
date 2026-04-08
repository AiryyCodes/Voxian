#pragma once

#include "Math/Vector.h"
#include "World/Entity/Entity.h"

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

class Chunk : public Entity
{
public:
    Chunk(Vector2i position);
    ~Chunk();

    bool GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, bool solid);

    bool IsBlockSolid(int x, int y, int z) const;

private:
    Vector2i m_Position;

    bool m_Blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE] = {{{false}}}; // Placeholder block data, all air for now
};