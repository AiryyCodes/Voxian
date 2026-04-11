#pragma once

#include "Math/Vector.h"
#include "World/Chunk/ChunkMesh.h"
#include "World/Chunk/ChunkSnapshot.h"
#include "World/Entity/Chunk.h"
#include "World/Entity/Component/Component.h"

#include <array>

class ChunkMeshGenerator : public Component
{
public:
    ChunkMeshGenerator(Chunk &chunk);
    ~ChunkMeshGenerator();

    ChunkMeshData GenerateMesh(const ChunkSnapshot &snapshot);

private:
    float GetOcclusion(bool side1, bool side2, bool corner);

    std::array<float, 4> GetVertexAOs(const ChunkSnapshot &snapshot, Vector3i blockPos, Vector3i normal);
    std::array<Vector3i, 3> GetAONeighbors(int vertexIndex, Vector3i face);

private:
    Chunk &m_Chunk;
};
