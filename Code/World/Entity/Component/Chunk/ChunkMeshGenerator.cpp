#include "ChunkMeshGenerator.h"
#include "Math/Vector.h"
#include "Util/Direction.h"
#include "World/Block/BlockProperties.h"
#include "World/Entity/Chunk.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct FaceIndices
{
    int v0, v1, v2, v3;
    Vector2f UVs[4];
};

ChunkMeshGenerator::ChunkMeshGenerator(Chunk &chunk)
    : m_Chunk(chunk)
{
}

ChunkMeshGenerator::~ChunkMeshGenerator()
{
}

ChunkMeshData ChunkMeshGenerator::GenerateMesh(const ChunkSnapshot &snapshot)
{
    ChunkMeshData meshData;
    meshData.Vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 4);
    meshData.Indices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 6);

    static const std::unordered_map<Direction, FaceIndices> faceIndicesMap = {
        {Direction::Up, {2, 6, 7, 3, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Down, {1, 5, 4, 0, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Left, {0, 2, 3, 1, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Right, {4, 5, 7, 6, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Forward, {1, 3, 7, 5, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Backward, {4, 6, 2, 0, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
    };

    // Corner positions of a unit cube — index matches FaceIndices v0-v3
    static const Vector3i corners[8] = {
        {0, 0, 0},
        {0, 0, 1},
        {0, 1, 0},
        {0, 1, 1},
        {1, 0, 0},
        {1, 0, 1},
        {1, 1, 0},
        {1, 1, 1},
    };

    static const std::unordered_map<Direction, int> normalIndexMap = {
        {Direction::Right, 0},
        {Direction::Left, 1},
        {Direction::Up, 2},
        {Direction::Down, 3},
        {Direction::Forward, 4},
        {Direction::Backward, 5},
    };

    for (int x = 0; x < CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < CHUNK_SIZE; ++z)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                uint16_t blockId = snapshot.GetBlock(x + 1, y + 1, z + 1);
                if (blockId == 0)
                    continue;

                const BlockProperties &properties = snapshot.GetBlockProperties(blockId);
                if (properties.IsAir)
                    continue;

                int textureIndex = blockId - 1;

                for (const auto &direction : Direction::GetAllDirections())
                {
                    Vector3i relativePos = Vector3i(x, y, z) + direction.ToVector();
                    uint16_t neighborId = snapshot.GetBlock(relativePos.x + 1, relativePos.y + 1, relativePos.z + 1);
                    const BlockProperties &neighborProperties = snapshot.GetBlockProperties(neighborId);

                    if (neighborId != 0 && !neighborProperties.IsAir)
                        continue;

                    const FaceIndices &face = faceIndicesMap.at(direction);
                    int normalIndex = normalIndexMap.at(direction);
                    unsigned int baseIndex = meshData.Vertices.size();

                    // Emit 4 packed vertices
                    for (int i = 0; i < 4; ++i)
                    {
                        int vi = (&face.v0)[i]; // v0, v1, v2, v3
                        Vector3i c = corners[vi];
                        meshData.Vertices.push_back(MakeVertex(
                            x + c.x, y + c.y, z + c.z,
                            normalIndex,
                            i, // corner index 0-3 maps to UV in shader
                            textureIndex));
                    }

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