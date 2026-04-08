#pragma once

#include "Math/Vector.h"
#include "World/Entity/Entity.h"

#define CHUNK_SIZE 16

class Chunk : public Entity
{
public:
    Chunk(Vector2i position);
    ~Chunk();
};