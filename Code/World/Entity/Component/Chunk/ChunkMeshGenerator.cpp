#include "ChunkMeshGenerator.h"
#include "Engine.h"
#include "Math/Vector.h"
#include "World/Block/BlockProperties.h"
#include "World/Chunk/ChunkMesh.h"
#include "World/Entity/Chunk.h"

#include <array>
#include <cstdint>
#include <vector>

ChunkMeshGenerator::ChunkMeshGenerator(Chunk &chunk)
    : m_Chunk(chunk)
{
}

ChunkMeshGenerator::~ChunkMeshGenerator()
{
}

ChunkMeshGroup ChunkMeshGenerator::GenerateMesh(const ChunkSnapshot &snapshot)
{
    ChunkMeshGroup group;
    group.Opaque.Vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 4);
    group.Opaque.Indices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 6);
    group.Transparent.Vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 4);
    group.Transparent.Indices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 6);

    static const std::array<Vector3i, 6> normalMap = {
        Vector3i(1, 0, 0),
        Vector3i(-1, 0, 0),
        Vector3i(0, 1, 0),
        Vector3i(0, -1, 0),
        Vector3i(0, 0, -1),
        Vector3i(0, 0, 1),
    };

    const BlockRegistry &blockRegistry = EngineContext::GetBlockRegistry();

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

                const BlockRenderData &render = blockRegistry.GetRenderData(blockId);
                const BakedModel *model = render.Model;

                if (!model)
                    continue;

                const Vector3f blockPos = Vector3f(x, y, z);

                ChunkMeshData &destData = (properties.Transparency == TransparencyType::Transparent)
                                              ? group.Transparent
                                              : group.Opaque;

                // Emit 4 packed vertices
                for (const auto &elem : model->Elements)
                {
                    for (const auto &bakedFace : elem.Faces)
                    {
                        if (bakedFace.CullFaceIndex >= 0)
                        {
                            Vector3i cullDir = normalMap[bakedFace.CullFaceIndex];
                            Vector3i neighborPos = Vector3i(x, y, z) + cullDir;
                            uint16_t neighborId = snapshot.GetBlock(neighborPos.x + 1, neighborPos.y + 1, neighborPos.z + 1);
                            const BlockProperties &neighborProps = snapshot.GetBlockProperties(neighborId);

                            if (neighborId != 0 && !neighborProps.IsAir &&
                                neighborProps.Transparency == TransparencyType::Opaque &&
                                neighborProps.IsFullCube)
                                continue;
                        }

                        int normalIndex = bakedFace.NormalIndex;

                        std::array<float, 4> ao;
                        for (int i = 0; i < 4; ++i)
                        {
                            Vector3f pos = blockPos + bakedFace.Positions[i];
                            if (elem.NoAmbientOcclusion)
                                ao[i] = 1.0f;
                            else
                                ao[i] = GetVertexAO(snapshot, pos, blockPos, normalMap[normalIndex]);
                        }

                        unsigned int baseIndex = destData.Vertices.size();

                        for (int i = 0; i < 4; ++i)
                        {
                            // Baked positions are in 0-1 block space, offset by block pos
                            Vector3f pos = blockPos + bakedFace.Positions[i];

                            destData.Vertices.push_back(MakeVertex(
                                pos.x, pos.y, pos.z,
                                normalIndex,
                                i,
                                bakedFace.TextureLayer,
                                ao[i],
                                bakedFace.UVs));
                        }

                        float diag1 = ao[0] + ao[2];
                        float diag2 = ao[1] + ao[3];

                        if (diag1 > diag2)
                        {
                            destData.Indices.push_back(baseIndex + 0);
                            destData.Indices.push_back(baseIndex + 1);
                            destData.Indices.push_back(baseIndex + 2);
                            destData.Indices.push_back(baseIndex + 0);
                            destData.Indices.push_back(baseIndex + 2);
                            destData.Indices.push_back(baseIndex + 3);
                        }
                        else
                        {
                            destData.Indices.push_back(baseIndex + 1);
                            destData.Indices.push_back(baseIndex + 2);
                            destData.Indices.push_back(baseIndex + 3);
                            destData.Indices.push_back(baseIndex + 1);
                            destData.Indices.push_back(baseIndex + 3);
                            destData.Indices.push_back(baseIndex + 0);
                        }
                    }
                }
            }
        }
    }

    return group;
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

float ChunkMeshGenerator::GetVertexAO(
    const ChunkSnapshot &snapshot, Vector3f vertexPos, Vector3i blockPos, Vector3i normal)
{
    Vector3i directPos = blockPos + normal;
    uint16_t directId = snapshot.GetBlock(directPos.x + 1, directPos.y + 1, directPos.z + 1);
    const auto &directP = snapshot.GetBlockProperties(directId);

    float baseAO = 1.0f;
    if (!directP.IsAir && directP.Transparency == TransparencyType::Cutout)
    {
        baseAO = 0.6f;
    }

    Vector3f localPos = vertexPos - Vector3f(blockPos + normal);
    Vector3i corner = Vector3i(
        (localPos.x > 0.5f - 0.001f) ? 1 : -1,
        (localPos.y > 0.5f - 0.001f) ? 1 : -1,
        (localPos.z > 0.5f - 0.001f) ? 1 : -1);

    Vector3i sideA, sideB;
    if (normal.x != 0)
    {
        sideA = {0, corner.y, 0};
        sideB = {0, 0, corner.z};
    }
    else if (normal.y != 0)
    {
        sideA = {corner.x, 0, 0};
        sideB = {0, 0, corner.z};
    }
    else
    {
        sideA = {corner.x, 0, 0};
        sideB = {0, corner.y, 0};
    }

    auto sample = [&](Vector3i offset) -> bool
    {
        Vector3i s = blockPos + normal + offset;
        uint16_t id = snapshot.GetBlock(s.x + 1, s.y + 1, s.z + 1);
        const auto &p = snapshot.GetBlockProperties(id);
        return !p.IsAir && (p.Transparency != TransparencyType::Transparent) && p.IsFullCube;
    };

    bool s1 = sample(sideA);
    bool s2 = sample(sideB);
    bool c = sample(sideA + sideB);

    float cornerAO = GetOcclusion(s1, s2, c);

    return baseAO * cornerAO;
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
