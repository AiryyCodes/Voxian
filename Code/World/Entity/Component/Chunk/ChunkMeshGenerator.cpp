#include "ChunkMeshGenerator.h"
#include "Math/Vector.h"
#include "Util/Direction.h"
#include "World/Block/BlockProperties.h"
#include "World/Entity/Chunk.h"

#include <array>
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
        {Direction::Left, {1, 0, 2, 3, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Right, {4, 5, 7, 6, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Forward, {6, 2, 0, 4, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
        {Direction::Backward, {3, 7, 5, 1, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
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

    static const std::array<Vector3i, 6> normalMap = {
        Vector3i(1, 0, 0),
        Vector3i(-1, 0, 0),
        Vector3i(0, 1, 0),
        Vector3i(0, -1, 0),
        Vector3i(0, 0, -1),
        Vector3i(0, 0, 1),
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

                    auto ao = GetVertexAOs(snapshot, Vector3i(x, y, z), normalMap[normalIndex]);

                    // Emit 4 packed vertices
                    for (int i = 0; i < 4; ++i)
                    {
                        int vi = (&face.v0)[i]; // v0, v1, v2, v3
                        Vector3i c = corners[vi];
                        meshData.Vertices.push_back(MakeVertex(
                            x + c.x, y + c.y, z + c.z,
                            normalIndex,
                            i, // corner index 0-3 maps to UV in shader
                            textureIndex,
                            ao[i]));
                    }

                    float diag1 = ao[0] + ao[2];
                    float diag2 = ao[1] + ao[3];

                    if (diag1 > diag2)
                    {
                        meshData.Indices.push_back(baseIndex + 0);
                        meshData.Indices.push_back(baseIndex + 1);
                        meshData.Indices.push_back(baseIndex + 2);
                        meshData.Indices.push_back(baseIndex + 0);
                        meshData.Indices.push_back(baseIndex + 2);
                        meshData.Indices.push_back(baseIndex + 3);
                    }
                    else
                    {
                        meshData.Indices.push_back(baseIndex + 1);
                        meshData.Indices.push_back(baseIndex + 2);
                        meshData.Indices.push_back(baseIndex + 3);
                        meshData.Indices.push_back(baseIndex + 1);
                        meshData.Indices.push_back(baseIndex + 3);
                        meshData.Indices.push_back(baseIndex + 0);
                    }
                }
            }
        }
    }

    return meshData;
}

float ChunkMeshGenerator::GetOcclusion(bool side1, bool side2, bool corner)
{
    if (side1 && side2 && corner)
        return 0.4f;
    if (side1 && side2)
        return 0.6f;
    if (side1 || side2 || corner)
        return 0.8f;
    return 1.0f;
}

std::array<float, 4> ChunkMeshGenerator::GetVertexAOs(const ChunkSnapshot &snapshot, Vector3i blockPos, Vector3i normal)
{
    std::array<float, 4> ao;
    for (int i = 0; i < 4; i++)
    {
        auto neighbors = GetAONeighbors(i, normal);

        const Vector3i &p1 = neighbors[0];
        const Vector3i &p2 = neighbors[1];
        const Vector3i &p3 = neighbors[2];

        bool side1 = !snapshot.GetBlockProperties(snapshot.GetBlock(blockPos.x + p1.x + 1, blockPos.y + p1.y + 1, blockPos.z + p1.z + 1)).IsAir;
        bool side2 = !snapshot.GetBlockProperties(snapshot.GetBlock(blockPos.x + p2.x + 1, blockPos.y + p2.y + 1, blockPos.z + p2.z + 1)).IsAir;
        bool corner = !snapshot.GetBlockProperties(snapshot.GetBlock(blockPos.x + p3.x + 1, blockPos.y + p3.y + 1, blockPos.z + p3.z + 1)).IsAir;

        ao[i] = GetOcclusion(side1, side2, corner);
    }

    if (normal == Vector3i(0, -1, 0))
    {
        std::swap(ao[0], ao[2]);
    }
    else if (normal == Vector3i(0, 0, -1)) // Forward
    {
        std::reverse(ao.begin(), ao.end());
    }
    else if (normal == Vector3i(0, 0, 1)) // Backward
    {
        std::reverse(ao.begin(), ao.end());
    }

    return ao;
}

std::array<Vector3i, 3> ChunkMeshGenerator::GetAONeighbors(int vertexIndex, Vector3i face)
{
    Vector3i up(0, 1, 0), down(0, -1, 0);
    Vector3i left(-1, 0, 0), right(1, 0, 0);
    Vector3i front(0, 0, 1), back(0, 0, -1);

    using Triple = std::array<Vector3i, 3>;
    std::array<Triple, 4> neighbors;

    if (face == Vector3i(1, 0, 0))
    {
        neighbors = {Triple{back + right, down + right, back + down + right},
                     Triple{front + right, down + right, front + down + right},
                     Triple{front + right, up + right, front + up + right},
                     Triple{back + right, up + right, back + up + right}};
    }
    else if (face == Vector3i(-1, 0, 0))
    {
        neighbors = {Triple{front + left, down + left, front + down + left},
                     Triple{back + left, down + left, back + down + left},
                     Triple{back + left, up + left, back + up + left},
                     Triple{front + left, up + left, front + up + left}};
    }
    else if (face == Vector3i(0, 1, 0))
    {
        neighbors = {Triple{left + up, back + up, left + back + up},
                     Triple{right + up, back + up, right + back + up},
                     Triple{right + up, front + up, right + front + up},
                     Triple{left + up, front + up, left + front + up}};
    }
    else if (face == Vector3i(0, -1, 0))
    {
        neighbors = {Triple{left + down, front + down, left + front + down},
                     Triple{right + down, front + down, right + front + down},
                     Triple{right + down, back + down, right + back + down},
                     Triple{left + down, back + down, left + back + down}};
    }
    else if (face == Vector3i(0, 0, -1)) // Forward
    {
        Vector3i f(0, 0, -1);
        neighbors = {Triple{right + f, down + f, right + down + f},
                     Triple{left + f, down + f, left + down + f},
                     Triple{left + f, up + f, left + up + f},
                     Triple{right + f, up + f, right + up + f}};
    }
    else if (face == Vector3i(0, 0, 1)) // Backward
    {
        Vector3i b(0, 0, 1);
        neighbors = {Triple{left + b, down + b, left + down + b},
                     Triple{right + b, down + b, right + down + b},
                     Triple{right + b, up + b, right + up + b},
                     Triple{left + b, up + b, left + up + b}};
    }
    else
    {
        throw std::runtime_error("Invalid face normal");
    }

    return neighbors[vertexIndex];
}
