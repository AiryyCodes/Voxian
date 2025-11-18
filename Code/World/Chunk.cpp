
#include "World/Chunk.h"
#include "World/ChunkManager.h"
#include "Logger.h"

#include <algorithm>
#include <glad/gl.h>
#include <FastNoiseLite.h>
#include <iostream>
#include <mutex>
#include <stdexcept>

Chunk::~Chunk()
{
    if (gpuReady)
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }
}

void Chunk::generateBlocks()
{
    FastNoiseLite noise(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.01f);

    float amplitude = 15.0f;

    for (int x = 0; x < CHUNK_WIDTH; ++x)
    {
        for (int z = 0; z < CHUNK_WIDTH; ++z)
        {
            // Use global coordinates so noise matches across chunks
            float worldX = static_cast<float>(chunkX * CHUNK_WIDTH + x);
            float worldZ = static_cast<float>(chunkZ * CHUNK_WIDTH + z);

            float height = noise.GetNoise(worldX, worldZ); // continuous noise
            int terrainHeight = static_cast<int>(CHUNK_BASE_HEIGHT + height * amplitude);

            for (int y = 0; y < CHUNK_BASE_HEIGHT; ++y)
            {
                // setBlock(x, y, z, true);
                setBlock(x, y, z, y < terrainHeight);
            }
        }
    }

    int blockX = CHUNK_WIDTH / 2;
    int blockY = CHUNK_BASE_HEIGHT;
    int blockZ = CHUNK_WIDTH / 2;

    // setBlock(blockX, blockY, blockZ, true);
}

void Chunk::generateMesh()
{
    MeshData newMesh;

    // Vertex offsets for a unit cube at origin
    glm::vec3 p000(0, 0, 0);
    glm::vec3 p001(0, 0, 1);
    glm::vec3 p010(0, 1, 0);
    glm::vec3 p011(0, 1, 1);
    glm::vec3 p100(1, 0, 0);
    glm::vec3 p101(1, 0, 1);
    glm::vec3 p110(1, 1, 0);
    glm::vec3 p111(1, 1, 1);

    glm::vec2 uv0(0, 1), uv1(1, 1), uv2(1, 0), uv3(0, 0);

    for (int x = 0; x < CHUNK_WIDTH; ++x)
    {
        for (int y = 0; y < CHUNK_HEIGHT; ++y)
        {
            for (int z = 0; z < CHUNK_WIDTH; ++z)
            {
                if (!getBlock(x, y, z))
                    continue;

                glm::vec3 blockPos(x, y, z);

                // Define each face: vertices, normal
                struct Face
                {
                    glm::vec3 v0, v1, v2, v3;
                    glm::vec3 normal;
                };

                Face faces[6] = {
                    // Right (+X)
                    {blockPos + p100, blockPos + p101, blockPos + p111, blockPos + p110, glm::ivec3(1, 0, 0)},
                    // Left (-X)
                    {blockPos + p001, blockPos + p000, blockPos + p010, blockPos + p011, glm::ivec3(-1, 0, 0)},
                    // Top (+Y)
                    {blockPos + p010, blockPos + p110, blockPos + p111, blockPos + p011, glm::ivec3(0, 1, 0)},
                    // Bottom (-Y)
                    {blockPos + p000, blockPos + p100, blockPos + p101, blockPos + p001, glm::ivec3(0, -1, 0)},
                    // Back (+Z)
                    {blockPos + p101, blockPos + p001, blockPos + p011, blockPos + p111, glm::ivec3(0, 0, 1)},
                    // Front (-Z)
                    {blockPos + p000, blockPos + p100, blockPos + p110, blockPos + p010, glm::ivec3(0, 0, -1)},
                };

                for (auto &face : faces)
                {
                    glm::ivec3 worldBlockPos = glm::ivec3(chunkX * CHUNK_WIDTH + x,
                                                          y,
                                                          chunkZ * CHUNK_WIDTH + z);

                    // Check neighbor in face normal direction
                    glm::ivec3 offset(static_cast<int>(face.normal.x),
                                      static_cast<int>(face.normal.y),
                                      static_cast<int>(face.normal.z));
                    int nx = worldBlockPos.x + offset.x;
                    int ny = worldBlockPos.y + offset.y;
                    int nz = worldBlockPos.z + offset.z;

                    bool neighborAir = (nx < 0 || nx >= CHUNK_WIDTH || ny < 0 || ny >= CHUNK_HEIGHT || nz < 0 || nz >= CHUNK_WIDTH) || !getBlock(nx, ny, nz);
                    if (!neighborAir)
                        continue;

                    auto ao = GetVertexAOs(worldBlockPos, glm::ivec3(face.normal));

                    unsigned int baseIndex = newMesh.vertices.size() / 9; // 9 floats per vertex

                    auto pushVertex = [&](glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, float aoValue)
                    {
                        newMesh.vertices.push_back(pos.x);
                        newMesh.vertices.push_back(pos.y);
                        newMesh.vertices.push_back(pos.z);
                        newMesh.vertices.push_back(normal.x);
                        newMesh.vertices.push_back(normal.y);
                        newMesh.vertices.push_back(normal.z);
                        newMesh.vertices.push_back(uv.x);
                        newMesh.vertices.push_back(uv.y);
                        newMesh.vertices.push_back(aoValue);
                    };

                    pushVertex(face.v0, face.normal, uv0, ao[0]);
                    pushVertex(face.v1, face.normal, uv1, ao[1]);
                    pushVertex(face.v2, face.normal, uv2, ao[2]);
                    pushVertex(face.v3, face.normal, uv3, ao[3]);

                    // Choose diagonal split based on AO
                    float diag1 = ao[0] + ao[2];
                    float diag2 = ao[1] + ao[3];

                    if (diag1 > diag2)
                    {
                        newMesh.indices.insert(newMesh.indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2,
                                                                       baseIndex, baseIndex + 2, baseIndex + 3});
                    }
                    else
                    {
                        newMesh.indices.insert(newMesh.indices.end(), {baseIndex + 1, baseIndex + 2, baseIndex + 3,
                                                                       baseIndex + 1, baseIndex + 3, baseIndex});
                    }
                }
            }
        }
    }

    {
        std::scoped_lock lock(meshMutex);
        mesh = std::move(newMesh);
    }

    meshReady = true;

    /*
    // mark neighbors for rebuild
    static const Vector2i offsets[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    if (!chunkManager)
        return;
    for (auto &off : offsets)
    {
        auto neighbor = chunkManager->GetChunk(chunkX + off.x, chunkZ + off.y);
        if (neighbor && neighbor->meshReady)
            neighbor->needsRebuild = true;
    }
    */
}

void Chunk::uploadMeshToGPU()
{
    if (gpuReady)
        return;

    std::scoped_lock lock(meshMutex);

    if (mesh.vertices.empty() || mesh.indices.empty())
        return;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    GLsizei stride = 9 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    gpuReady = true;
}

void Chunk::draw(const Shader &shader)
{
    if (!gpuReady)
        return;

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool Chunk::IsSolid(int wx, int wy, int wz) const
{
    if (wy < 0 || wy >= CHUNK_HEIGHT)
        return false;

    int localX = wx - chunkX * CHUNK_WIDTH;
    int localZ = wz - chunkZ * CHUNK_WIDTH;

    int neighborX = chunkX;
    int neighborZ = chunkZ;

    if (localX < 0)
    {
        neighborX -= 1;
        localX += CHUNK_WIDTH;
    }
    if (localX >= CHUNK_WIDTH)
    {
        neighborX += 1;
        localX -= CHUNK_WIDTH;
    }
    if (localZ < 0)
    {
        neighborZ -= 1;
        localZ += CHUNK_WIDTH;
    }
    if (localZ >= CHUNK_WIDTH)
    {
        neighborZ += 1;
        localZ -= CHUNK_WIDTH;
    }

    auto neighbor = chunkManager->GetChunk(neighborX, neighborZ);
    if (!neighbor)
        return true; // treat unloaded neighbor as solid for AO

    return neighbor->getBlock(localX, wy, localZ);
}

std::array<float, 4> Chunk::GetVertexAOs(const Vector3i &blockPos, const Vector3i &faceNormal)
{
    auto getOcclusion = [](bool s1, bool s2, bool c) -> float
    {
        if (s1 && s2 && c)
            return 0.7f;
        if (s1 && s2)
            return 0.8f;
        if (s1 || s2 || c)
            return 0.9f;
        return 1.0f;
    };

    std::array<float, 4> ao;

    for (int i = 0; i < 4; i++)
    {
        auto neighbors = GetAONeighbors(i, faceNormal);

        const Vector3i &p1 = neighbors[0];
        const Vector3i &p2 = neighbors[1];
        const Vector3i &p3 = neighbors[2];

        bool s1 = IsSolid(blockPos.x + p1.x, blockPos.y + p1.y, blockPos.z + p1.z);
        bool s2 = IsSolid(blockPos.x + p2.x, blockPos.y + p2.y, blockPos.z + p2.z);
        bool c = IsSolid(blockPos.x + p3.x, blockPos.y + p3.y, blockPos.z + p3.z);

        ao[i] = getOcclusion(s1, s2, c);
    }

    if (faceNormal == Vector3i(0, -1, 0))
        std::swap(ao[1], ao[3]);

    const int DEBUG_X = CHUNK_WIDTH / 2;
    const int DEBUG_Y = CHUNK_BASE_HEIGHT;
    const int DEBUG_Z = CHUNK_WIDTH / 2;

    return ao;
}

std::array<Vector3i, 3> Chunk::GetAONeighbors(int vertexIndex, const Vector3i &face)
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
    else if (face == Vector3i(0, 0, 1))
    {
        neighbors = {Triple{right + front, down + front, right + down + front},
                     Triple{left + front, down + front, left + down + front},
                     Triple{left + front, up + front, left + up + front},
                     Triple{right + front, up + front, right + up + front}};
    }
    else if (face == Vector3i(0, 0, -1))
    {
        neighbors = {Triple{left + back, down + back, left + down + back},
                     Triple{right + back, down + back, right + down + back},
                     Triple{right + back, up + back, right + up + back},
                     Triple{left + back, up + back, left + up + back}};
    }
    else
    {
        throw std::runtime_error("invalid face normal");
    }

    return neighbors[vertexIndex];
}

bool Chunk::CanGenerateMesh() const
{
    return blocksReady;
}

void Chunk::TryGenerateMesh()
{
    bool expected = false;
    if (!blocksReady || !isGeneratingMesh.compare_exchange_strong(expected, true))
        return;

    auto self = shared_from_this();
    chunkManager->m_ThreadPool.post([self]()
                                    {
        self->generateMesh();
        self->isGeneratingMesh = false; });
}

void Chunk::applyBlockData(BlockData &data)
{
    blocks = data.blocks;

    blocksReady = true;
}

void Chunk::applyMeshData(MeshData &data)
{
    mesh.vertices = data.vertices;
    mesh.indices = data.indices;

    meshReady = true;
    gpuReady = false;
}
