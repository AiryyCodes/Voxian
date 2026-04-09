#include "ChunkMeshGenerator.h"
#include "Engine.h"
#include "Logger.h"
#include "Math/Vector.h"
#include "Util/Direction.h"
#include "World/Entity/Chunk.h"

#include <cstdint>
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

    Chunk &chunk = static_cast<Chunk &>(GetOwner());

    static const std::unordered_map<Direction, FaceIndices> faceIndicesMap = {
        {Direction::Up, {2, 6, 7, 3}},
        {Direction::Down, {1, 5, 4, 0}},
        {Direction::Left, {0, 2, 3, 1}},
        {Direction::Right, {4, 5, 7, 6}},
        {Direction::Forward, {1, 3, 7, 5}},
        {Direction::Backward, {4, 6, 2, 0}}};

    for (int x = 0; x < CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < CHUNK_SIZE; ++z)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                Vector3i blockPos = Vector3f(x, y, z);

                Vector3f points[8] = {
                    blockPos + Vector3i(0.0f, 0.0f, 0.0f),
                    blockPos + Vector3i(0.0f, 0.0f, blockSize.z),
                    blockPos + Vector3i(0.0f, blockSize.y, 0.0f),
                    blockPos + Vector3i(0.0f, blockSize.y, blockSize.z),
                    blockPos + Vector3i(blockSize.x, 0.0f, 0.0f),
                    blockPos + Vector3i(blockSize.x, 0.0f, blockSize.z),
                    blockPos + Vector3i(blockSize.x, blockSize.y, 0.0f),
                    blockPos + Vector3i(blockSize.x, blockSize.y, blockSize.z),
                };

                uint16_t blockId = chunk.GetBlock(x, y, z);
                if (blockId == 0)
                    continue; // Skip air blocks

                const Block *block = EngineContext::GetBlockRegistry().GetBlockById(blockId);
                if (!block)
                    continue;

                if (block->GetProperties().IsAir())
                    continue; // Skip air blocks

                for (const auto &direction : Direction::GetAllDirections())
                {
                    Vector3i relativePos = blockPos + direction.ToVector();
                    uint16_t neighborId = chunk.GetBlock(relativePos.x, relativePos.y, relativePos.z);

                    const Block *neighborBlock = EngineContext::GetBlockRegistry().GetBlockById(neighborId);
                    if (!neighborBlock)
                        continue;

                    bool isFaceVisible = (neighborId == 0) || neighborBlock->GetProperties().IsAir();

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
                    meshData.Indices.push_back(baseIndex + 2);
                    meshData.Indices.push_back(baseIndex + 1);
                    meshData.Indices.push_back(baseIndex);
                    meshData.Indices.push_back(baseIndex + 3);
                    meshData.Indices.push_back(baseIndex + 2);
                }
            }
        }
    }

    return meshData;
}