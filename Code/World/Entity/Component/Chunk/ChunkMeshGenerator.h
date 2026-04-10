#pragma once

#include "World/Chunk/ChunkMesh.h"
#include "World/Entity/Chunk.h"
#include "World/Entity/Component/Component.h"

class ChunkMeshGenerator : public Component
{
public:
    ChunkMeshGenerator(Chunk &chunk);
    ~ChunkMeshGenerator();

    ChunkMeshData GenerateMesh(const ChunkSnapshot &snapshot);

private:
    Chunk &m_Chunk;
};