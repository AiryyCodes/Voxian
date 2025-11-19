
#include "World/ChunkManager.h"
#include "FastNoiseLite.h"
#include "Graphics/Shader.h"
#include "Logger.h"
#include "Math/Vector.h"
#include "Queue.h"
#include "World/Chunk.h"

#include <algorithm>
#include <memory>
#include <stdexcept>

void ChunkManager::RequestChunk(int x, int z)
{
    Vector2i key{x, z};
    {
        std::scoped_lock lock(m_ChunkMutex);
        if (m_Chunks.find(key) != m_Chunks.end() || m_RequestedChunks.count(key))
            return; // already loaded or requested
        m_RequestedChunks.insert(key);
    }

    auto chunk = std::make_shared<Chunk>(this, x, z);

    {
        std::scoped_lock lock(m_ChunkMutex);
        m_Chunks[key] = chunk;
    }

    // Use weak_ptr to avoid use-after-free
    std::weak_ptr<Chunk> weakChunk = chunk;

    // Generate blocks and mesh asynchronously
    m_ThreadPool.post([this, weakChunk, key]()
                      {
        if (auto chunk = weakChunk.lock())  // Only proceed if chunk still exists
        {
            BlockData blockData = GenerateBlocks(chunk->m_Position.x, chunk->m_Position.y);

            m_BlockQueue.push({
                .callback = [weakChunk](BlockData &data) {
                    if (auto chunk = weakChunk.lock())
                        chunk->SetBlockData(data);
                },
                .result = std::move(blockData)
            });

            MeshData meshData = GenerateMesh(BlockData{chunk->m_Blocks}, chunk->m_Position.x, chunk->m_Position.y);

            m_MeshQueue.push({
                .callback = [this, weakChunk](MeshData &mesh) {
                    if (auto chunk = weakChunk.lock())
                    {
                        chunk->SetMeshData(mesh);

                        // Mark neighbors as dirty
                        static const Vector2i offsets[] = {{1,0},{-1,0},{0,1},{0,-1}};
                        for (auto &off : offsets)
                        {
                            auto neighbor = GetChunk(chunk->m_Position.x + off.x, chunk->m_Position.y + off.y);
                            if (neighbor)
                                neighbor->m_NeedsRebuild = true;
                        }
                    }
                },
                .result = std::move(meshData)
            });
        } });
}

void ChunkManager::UpdatePlayerPosition(const Vector3 &pos)
{
    m_PlayerPosition = pos;
    Vector2i playerChunk{
        static_cast<int>(std::floor(pos.x / CHUNK_WIDTH)),
        static_cast<int>(std::floor(pos.z / CHUNK_WIDTH))};

    // Compute desired chunks with distance
    std::vector<std::pair<int, Vector2i>> desiredChunks; // distance, coords
    for (int dz = -m_ViewDistance; dz <= m_ViewDistance; dz++)
    {
        for (int dx = -m_ViewDistance; dx <= m_ViewDistance; dx++)
        {
            Vector2i c{playerChunk.x + dx, playerChunk.y + dz};
            int dist2 = dx * dx + dz * dz;
            desiredChunks.emplace_back(dist2, c);
        }
    }

    // Sort by distance so closest chunks are requested first
    std::sort(desiredChunks.begin(), desiredChunks.end(),
              [](const auto &a, const auto &b)
              { return a.first < b.first; });

    // Request new chunks in order
    for (const auto &[_, key] : desiredChunks)
        RequestChunk(key.x, key.y);

    // Mark far-away chunks for unloading
    std::scoped_lock lock(m_ChunkMutex);
    for (auto &[pos, chunk] : m_Chunks)
    {
        chunk->m_ShouldUnload = !std::any_of(desiredChunks.begin(), desiredChunks.end(),
                                             [&](const auto &p)
                                             { return p.second == pos; });
    }
}

void ChunkManager::Update(const Shader &shader)
{
    // Apply block results
    ThreadTask<BlockData> blockTask;
    while (m_BlockQueue.pop(blockTask))
        blockTask.callback(blockTask.result);

    // Apply mesh results
    ThreadTask<MeshData> meshTask;
    while (m_MeshQueue.pop(meshTask))
        meshTask.callback(meshTask.result);

    int px = static_cast<int>(std::floor(m_PlayerPosition.x / CHUNK_WIDTH));
    int pz = static_cast<int>(std::floor(m_PlayerPosition.z / CHUNK_WIDTH));
    Vector2i playerChunk{px, pz};

    // Rebuild dirty chunks
    std::vector<std::shared_ptr<Chunk>> dirtyChunks;
    {
        std::scoped_lock lock(m_ChunkMutex);
        for (auto &[pos, chunk] : m_Chunks)
        {
            if (chunk->m_NeedsRebuild && chunk->m_BlocksReady)
                dirtyChunks.push_back(chunk);
        }
    }

    std::sort(dirtyChunks.begin(), dirtyChunks.end(),
              [&](const std::shared_ptr<Chunk> &a, const std::shared_ptr<Chunk> &b)
              {
                  int dxA = a->m_Position.x - playerChunk.x;
                  int dzA = a->m_Position.y - playerChunk.y;
                  int dxB = b->m_Position.x - playerChunk.x;
                  int dzB = b->m_Position.y - playerChunk.y;
                  return (dxA * dxA + dzA * dzA) < (dxB * dxB + dzB * dzB);
              });

    int rebuildCount = 0;
    for (auto &chunk : dirtyChunks)
    {
        if (rebuildCount >= m_MaxRebuildsPerFrame)
            break;

        chunk->m_NeedsRebuild = false;

        m_ThreadPool.post([this, chunk]()
                          {
            MeshData newMesh = GenerateMesh(BlockData{chunk->m_Blocks}, chunk->m_Position.x, chunk->m_Position.y);
            m_MeshQueue.push({.callback = [chunk](MeshData &mesh) { chunk->SetMeshData(mesh); },
                              .result = std::move(newMesh)}); });

        rebuildCount++;
    }

    // Upload meshes, render, unload
    const int renderDistance = m_ViewDistance;

    std::scoped_lock lock(m_ChunkMutex);
    for (auto it = m_Chunks.begin(); it != m_Chunks.end();)
    {
        auto &chunk = it->second;
        int dx = chunk->m_Position.x - playerChunk.x;
        int dz = chunk->m_Position.y - playerChunk.y;
        int dist2 = dx * dx + dz * dz;

        if (chunk->m_MeshReady && !chunk->m_GpuReady)
            chunk->UploadMeshToGPU();

        if (chunk->m_GpuReady && dist2 <= renderDistance * renderDistance)
            chunk->Draw(shader);

        if (chunk->m_ShouldUnload && !chunk->m_NeedsRebuild && chunk->m_GpuReady)
        {
            chunk->DeleteGPUData();
            m_RequestedChunks.erase(it->first);
            it = m_Chunks.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // LOG_INFO("Chunks: {}", m_Chunks.size());
}

std::shared_ptr<Chunk> ChunkManager::GetChunk(int x, int z)
{
    std::scoped_lock lock(m_ChunkMutex);
    auto it = m_Chunks.find(Vector2i(x, z));
    return it != m_Chunks.end() ? it->second : nullptr;
}

bool ChunkManager::GetBlock(int x, int y, int z, int chunkX, int chunkZ, const BlockData &localData)
{
    // First, check if it's inside the local chunk
    if (x >= 0 && x < CHUNK_WIDTH &&
        y >= 0 && y < CHUNK_HEIGHT &&
        z >= 0 && z < CHUNK_WIDTH)
    {
        return localData.Get(x, y, z);
    }

    // Determine neighbor chunk coordinates
    int neighborChunkX = chunkX + (x < 0 ? -1 : (x >= CHUNK_WIDTH ? 1 : 0));
    int neighborChunkZ = chunkZ + (z < 0 ? -1 : (z >= CHUNK_WIDTH ? 1 : 0));

    // Wrap local coordinates into neighbor chunk
    int nx = (x + CHUNK_WIDTH) % CHUNK_WIDTH;
    int nz = (z + CHUNK_WIDTH) % CHUNK_WIDTH;

    auto neighbor = GetChunk(neighborChunkX, neighborChunkZ);
    if (!neighbor || !neighbor->m_BlocksReady)
        return false;

    std::lock_guard<std::mutex> lock(neighbor->m_Mutex);
    return neighbor->GetBlock(nx, y, nz);
}

BlockData ChunkManager::GenerateBlocks(int chunkX, int chunkZ)
{
    FastNoiseLite noise(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.01f);

    float amplitude = 15.0f;

    BlockData data;

    for (int x = 0; x < CHUNK_WIDTH; ++x)
    {
        for (int z = 0; z < CHUNK_WIDTH; ++z)
        {
            float worldX = static_cast<float>(chunkX * CHUNK_WIDTH + x);
            float worldZ = static_cast<float>(chunkZ * CHUNK_WIDTH + z);

            float height = noise.GetNoise(worldX, worldZ);
            int terrainHeight = static_cast<int>(CHUNK_BASE_HEIGHT + (height * amplitude));

            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                if (y < terrainHeight)
                    data.Blocks[GetBlockIndex(x, y, z)] = true;
            }
        }
    }

    return data;
}

MeshData ChunkManager::GenerateMesh(const BlockData &blockData, int chunkX, int chunkZ)
{
    MeshData data;

    // Vertex offsets for a unit cube at origin
    Vector3f p000(0, 0, 0);
    glm::vec3 p001(0, 0, 1);
    glm::vec3 p010(0, 1, 0);
    glm::vec3 p011(0, 1, 1);
    glm::vec3 p100(1, 0, 0);
    glm::vec3 p101(1, 0, 1);
    glm::vec3 p110(1, 1, 0);
    glm::vec3 p111(1, 1, 1);

    glm::vec2 uv0(0, 1), uv1(1, 1), uv2(1, 0), uv3(0, 0);

    auto isAir = [&](int x, int y, int z) -> bool
    {
        if (x < 0 || x >= CHUNK_WIDTH ||
            y < 0 || y >= CHUNK_HEIGHT ||
            z < 0 || z >= CHUNK_WIDTH)
            return true;
        return !blockData.Blocks[x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y)];
    };

    for (int x = 0; x < CHUNK_WIDTH; ++x)
    {
        for (int y = 0; y < CHUNK_HEIGHT; ++y)
        {
            for (int z = 0; z < CHUNK_WIDTH; ++z)
            {
                if (isAir(x, y, z))
                    continue;

                glm::vec3 blockPos(x, y, z);

                struct Face
                {
                    glm::vec3 v0, v1, v2, v3;
                    glm::ivec3 normal;
                };

                Face faces[6] = {
                    {blockPos + p100, blockPos + p101, blockPos + p111, blockPos + p110, glm::ivec3(1, 0, 0)},
                    {blockPos + p001, blockPos + p000, blockPos + p010, blockPos + p011, glm::ivec3(-1, 0, 0)},
                    {blockPos + p010, blockPos + p110, blockPos + p111, blockPos + p011, glm::ivec3(0, 1, 0)},
                    {blockPos + p000, blockPos + p100, blockPos + p101, blockPos + p001, glm::ivec3(0, -1, 0)},
                    {blockPos + p101, blockPos + p001, blockPos + p011, blockPos + p111, glm::ivec3(0, 0, 1)},
                    {blockPos + p000, blockPos + p100, blockPos + p110, blockPos + p010, glm::ivec3(0, 0, -1)},
                };

                for (auto &face : faces)
                {
                    int nx = x + face.normal.x;
                    int ny = y + face.normal.y;
                    int nz = z + face.normal.z;

                    if (!isAir(nx, ny, nz))
                        continue;

                    // AO calculation using only local neighbors
                    auto ao = GetVertexAOs(blockData, glm::ivec3(x, y, z), face.normal, Vector2i(chunkX, chunkZ));

                    unsigned int baseIndex = data.Vertices.size() / 9; // 9 floats per vertex

                    auto pushVertex = [&](glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, float aoValue)
                    {
                        data.Vertices.push_back(pos.x);
                        data.Vertices.push_back(pos.y);
                        data.Vertices.push_back(pos.z);
                        data.Vertices.push_back(normal.x);
                        data.Vertices.push_back(normal.y);
                        data.Vertices.push_back(normal.z);
                        data.Vertices.push_back(uv.x);
                        data.Vertices.push_back(uv.y);
                        data.Vertices.push_back(aoValue);
                    };

                    pushVertex(face.v0, face.normal, uv0, ao[0]);
                    pushVertex(face.v1, face.normal, uv1, ao[1]);
                    pushVertex(face.v2, face.normal, uv2, ao[2]);
                    pushVertex(face.v3, face.normal, uv3, ao[3]);

                    float diag1 = ao[0] + ao[2];
                    float diag2 = ao[1] + ao[3];

                    if (diag1 > diag2)
                        data.Indices.insert(data.Indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2,
                                                                 baseIndex, baseIndex + 2, baseIndex + 3});
                    else
                        data.Indices.insert(data.Indices.end(), {baseIndex + 1, baseIndex + 2, baseIndex + 3,
                                                                 baseIndex + 1, baseIndex + 3, baseIndex});
                }
            }
        }
    }

    return data;
}

int ChunkManager::GetBlockIndex(int x, int y, int z) const
{
    return x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y);
}

std::array<float, 4> ChunkManager::GetVertexAOs(const BlockData &localData, const Vector3i &blockPos, const Vector3i &faceNormal, const Vector2i chunkPos)
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

        bool s1 = GetBlock(blockPos.x + p1.x, blockPos.y + p1.y, blockPos.z + p1.z, chunkPos.x, chunkPos.y, localData);
        bool s2 = GetBlock(blockPos.x + p2.x, blockPos.y + p2.y, blockPos.z + p2.z, chunkPos.x, chunkPos.y, localData);
        bool c = GetBlock(blockPos.x + p3.x, blockPos.y + p3.y, blockPos.z + p3.z, chunkPos.x, chunkPos.y, localData);

        ao[i] = getOcclusion(s1, s2, c);
    }

    if (faceNormal == Vector3i(0, -1, 0))
        std::swap(ao[1], ao[3]);

    return ao;
}

std::array<Vector3i, 3> ChunkManager::GetAONeighbors(int vertexIndex, const Vector3i &face)
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
        throw std::runtime_error("Invalid face normal");
    }

    return neighbors[vertexIndex];
}
