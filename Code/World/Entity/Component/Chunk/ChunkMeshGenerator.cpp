#include "ChunkMeshGenerator.h"
#include "Math/Vector.h"
#include "Renderer/Mesh.h"
#include "Util/Direction.h"

#include <unordered_map>
#include <vector>

struct FaceIndices
{
    int v0, v1, v2, v3;
};

ChunkMeshGenerator::ChunkMeshGenerator(Chunk &chunk)
    : m_Chunk(chunk)
{
}

ChunkMeshGenerator::~ChunkMeshGenerator()
{
}

ChunkMeshData ChunkMeshGenerator::GenerateMesh()
{
    ChunkMeshData meshData;
    meshData.Vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

    Vector3f blockSize = Vector3f(1.0f, 1.0f, 1.0f); // Placeholder block size

    static const std::unordered_map<Direction, FaceIndices> faceIndicesMap = {
        {Direction::Up, {2, 6, 7, 3}},
        {Direction::Down, {1, 5, 4, 0}},
        {Direction::Left, {0, 4, 6, 2}},
        {Direction::Right, {5, 1, 3, 7}},
        {Direction::Forward, {1, 0, 2, 3}},
        {Direction::Backward, {4, 5, 7, 6}}};

    for (int x = 0; x < CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < CHUNK_SIZE; ++z)
        {
            for (int y = 0; y < CHUNK_SIZE; ++y)
            {
                Vector3f blockPos = Vector3f(x, y, z);

                Vector3f points[8] = {
                    blockPos + Vector3f(0.0f, 0.0f, 0.0f),
                    blockPos + Vector3f(0.0f, 0.0f, blockSize.z),
                    blockPos + Vector3f(0.0f, blockSize.y, 0.0f),
                    blockPos + Vector3f(0.0f, blockSize.y, blockSize.z),
                    blockPos + Vector3f(blockSize.x, 0.0f, 0.0f),
                    blockPos + Vector3f(blockSize.x, 0.0f, blockSize.z),
                    blockPos + Vector3f(blockSize.x, blockSize.y, 0.0f),
                    blockPos + Vector3f(blockSize.x, blockSize.y, blockSize.z),
                };

                for (const auto &direction : Direction::GetAllDirections())
                {
                    bool isFaceVisible = true; // Placeholder visibility check

                    if (!isFaceVisible)
                        continue;

                    FaceIndices faceIndices = faceIndicesMap.at(direction);
                    unsigned int baseIndex = meshData.Vertices.size();

                    ChunkVertex v0;
                    v0.Position = points[faceIndices.v0];
                    v0.Normal = direction.ToVector();
                    meshData.Vertices.push_back(v0);

                    ChunkVertex v1;
                    v1.Position = points[faceIndices.v1];
                    v1.Normal = direction.ToVector();
                    meshData.Vertices.push_back(v1);

                    ChunkVertex v2;
                    v2.Position = points[faceIndices.v2];
                    v2.Normal = direction.ToVector();
                    meshData.Vertices.push_back(v2);

                    ChunkVertex v3;
                    v3.Position = points[faceIndices.v3];
                    v3.Normal = direction.ToVector();
                    meshData.Vertices.push_back(v3);

                    meshData.Indices.push_back(baseIndex);
                    meshData.Indices.push_back(baseIndex + 1);
                    meshData.Indices.push_back(baseIndex + 2);
                    meshData.Indices.push_back(baseIndex);
                    meshData.Indices.push_back(baseIndex + 2);
                    meshData.Indices.push_back(baseIndex + 3);
                }
            }
        }
    }

    return meshData;
}