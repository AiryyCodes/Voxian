#pragma once

#include "Math/Vector.h"
#include "World/Entity/Chunk.h"
#include "World/Entity/Component/Component.h"
#include <vector>

struct ChunkVertex
{
    Vector3f Position;
    Vector3f Normal;
};

struct ChunkMeshData
{
    std::vector<ChunkVertex> Vertices;
    std::vector<unsigned int> Indices;
};

class ChunkMeshGenerator : public Component
{
public:
    ChunkMeshGenerator(Chunk &chunk);
    ~ChunkMeshGenerator();

    ChunkMeshData GenerateMesh();

private:
    Chunk &m_Chunk;
};