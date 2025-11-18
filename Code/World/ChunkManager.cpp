
// =====================================================================
// INTERNAL HELPERS
// =====================================================================
#include "World/ChunkManager.h"
#include "FastNoiseLite.h"
#include "Graphics/Shader.h"
#include "Math/Vector.h"
#include "Queue.h"
#include "World/Chunk.h"
#include <algorithm>
#include <memory>
#include <stdexcept>

static Vector2i ToChunkCoord(int x, int z)
{
    return Vector2i{
        x / CHUNK_WIDTH,
        z / CHUNK_WIDTH,
    };
}

// =====================================================================
// Get or create a chunk
// =====================================================================
std::shared_ptr<Chunk> ChunkManager::GetChunk(int cx, int cz)
{
    std::scoped_lock lock(m_ChunkMutex);
    Vector2i key{cx, cz};
    auto it = m_Chunks.find(key);
    if (it != m_Chunks.end())
        return it->second;
    return nullptr;
}

// =====================================================================
// Request chunk generation
// =====================================================================
// Schedule block + mesh generation on worker
void ChunkManager::RequestChunk(int cx, int cz)
{
    std::shared_ptr<Chunk> chunk;

    {
        std::scoped_lock lock(m_ChunkMutex);
        Vector2i key{cx, cz};
        if (m_Chunks.find(key) != m_Chunks.end())
            return;

        chunk = std::make_shared<Chunk>(this, cx, cz);
        m_Chunks[key] = chunk;
    }

    // Post worker to thread pool
    m_ThreadPool.post([this, chunk]()
                      {
        // --------------------------
        // PHASE 1: Generate blocks
        // --------------------------
        BlockData blockData = generateBlocks(chunk->chunkX, chunk->chunkZ);

        // Push block data to main thread
        blockQueue.push({.callback = [this, chunk](BlockData &data) {
            chunk->applyBlockData(data);
        }, .result = std::move(blockData)});

        // --------------------------
        // PHASE 2: Generate mesh
        // --------------------------
        // Note: we include neighbors for AO, but don't rebuild neighbors here
        MeshData meshData = generateMesh(BlockData{chunk->blocks}, chunk->chunkX, chunk->chunkZ);

        meshQueue.push({.callback = [this, chunk](MeshData &mesh)
        {
            chunk->applyMeshData(mesh);

            // Mark neighbors as dirty for next frame (deferred)
            static const Vector2i offsets[] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (auto &off : offsets)
            {
                auto neighbor = getChunk(chunk->chunkX + off.x, chunk->chunkZ + off.y);
                if (neighbor)
                    neighbor->needsRebuild = true; // just mark, don't rebuild now
            }
        }, .result = std::move(meshData)}); });
}

// =====================================================================
// Update from player movement
// =====================================================================
void ChunkManager::UpdatePlayerPosition(const Vector3 &pos)
{
    m_PlayerPosition = pos;

    Vector2i center{pos.x / CHUNK_WIDTH, pos.z / CHUNK_WIDTH};
    for (int dz = -viewDistance; dz <= viewDistance; dz++)
        for (int dx = -viewDistance; dx <= viewDistance; dx++)
            RequestChunk(center.x + dx, center.y + dz);
}

// =====================================================================
// Update – upload mesh + draw
// Call this every frame from main thread
// =====================================================================
void ChunkManager::Update(const Shader &shader)
{
    // Apply block results
    ThreadTask<BlockData> blockTask;
    while (blockQueue.pop(blockTask))
        blockTask.callback(blockTask.result);

    // Apply mesh results
    ThreadTask<MeshData> meshTask;
    while (meshQueue.pop(meshTask))
        meshTask.callback(meshTask.result);

    // Rebuild dirty chunks
    std::vector<std::shared_ptr<Chunk>> dirtyChunks;
    dirtyChunks.reserve(m_Chunks.size());

    for (auto &[pos, chunk] : m_Chunks)
    {
        if (chunk->needsRebuild && chunk->blocksReady)
        {
            dirtyChunks.push_back(chunk);
        }
    }

    int px = static_cast<int>(std::floor(m_PlayerPosition.x / CHUNK_WIDTH));
    int pz = static_cast<int>(std::floor(m_PlayerPosition.z / CHUNK_WIDTH));
    Vector2i playerChunk{px, pz};

    std::sort(dirtyChunks.begin(), dirtyChunks.end(),
              [&](const std::shared_ptr<Chunk> &a, const std::shared_ptr<Chunk> &b)
              {
                  int dxA = a->chunkX - playerChunk.x;
                  int dzA = a->chunkZ - playerChunk.y;
                  int dxB = b->chunkX - playerChunk.x;
                  int dzB = b->chunkZ - playerChunk.y;

                  return (dxA * dxA + dzA * dzA) < (dxB * dxB + dzB * dzB); // closer first
              });

    const int maxRebuildsPerFrame = 2;

    int rebuildCount = 0;
    for (auto &chunk : dirtyChunks)
    {
        if (rebuildCount >= maxRebuildsPerFrame)
            break;

        chunk->needsRebuild = false;

        m_ThreadPool.post([this, chunk]()
                          {
        MeshData newMesh = generateMesh(BlockData{chunk->blocks}, chunk->chunkX, chunk->chunkZ);
        meshQueue.push({.callback = [chunk](MeshData &mesh)
        {
            chunk->applyMeshData(mesh);
        }, .result = std::move(newMesh)}); });

        rebuildCount++;
    }

    // Upload meshes to GPU and render
    for (auto &[pos, chunk] : m_Chunks)
    {
        if (chunk->meshReady && !chunk->gpuReady)
            chunk->uploadMeshToGPU();

        if (chunk->gpuReady)
        {
            glm::vec3 chunkWorldPos{pos.x * CHUNK_WIDTH, 0.0f, pos.y * CHUNK_WIDTH};
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkWorldPos);
            shader.SetUniform("u_Model", model);
            chunk->draw(shader);
        }
    }
}

std::shared_ptr<Chunk> ChunkManager::getChunk(int x, int z)
{
    auto it = m_Chunks.find(Vector2i(x, z));
    return it != m_Chunks.end() ? it->second : nullptr;
}

bool ChunkManager::blockAtSafe(const BlockData &data, int x, int y, int z) const
{
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_WIDTH)
        return false;
    return data.blocks[getBlockIndex(x, y, z)];
}

bool ChunkManager::isBlockSolidGlobal(int x, int y, int z, int chunkX, int chunkZ, const BlockData &localData)
{
    // Local coordinates inside this chunk
    if (x >= 0 && x < CHUNK_WIDTH &&
        y >= 0 && y < CHUNK_HEIGHT &&
        z >= 0 && z < CHUNK_WIDTH)
    {
        return localData.blocks[getBlockIndex(x, y, z)];
    }

    // Out of bounds → check neighbor chunk
    int nx = x, nz = z;
    int neighborChunkX = chunkX;
    int neighborChunkZ = chunkZ;

    if (x < 0)
    {
        neighborChunkX -= 1;
        nx = x + CHUNK_WIDTH;
    }
    else if (x >= CHUNK_WIDTH)
    {
        neighborChunkX += 1;
        nx = x - CHUNK_WIDTH;
    }

    if (z < 0)
    {
        neighborChunkZ -= 1;
        nz = z + CHUNK_WIDTH;
    }
    else if (z >= CHUNK_WIDTH)
    {
        neighborChunkZ += 1;
        nz = z - CHUNK_WIDTH;
    }

    auto neighbor = getChunk(neighborChunkX, neighborChunkZ);
    if (!neighbor || !neighbor->blocksReady)
        return false; // treat as air until neighbor exists

    return neighbor->blocks[getBlockIndex(nx, y, nz)];
}

BlockData ChunkManager::generateBlocks(int chunkX, int chunkZ)
{
    FastNoiseLite noise(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.01f);

    float amplitude = 15.0f;

    BlockData data;

    for (int x = 0; x < CHUNK_WIDTH; ++x)
    {
        for (int z = 0; z < CHUNK_WIDTH; ++z)
        {
            // Use global coordinates so noise matches across chunks
            float worldX = static_cast<float>(chunkX * CHUNK_WIDTH + x);
            float worldZ = static_cast<float>(chunkZ * CHUNK_WIDTH + z);

            float height = noise.GetNoise(worldX, worldZ); // continuous noise
            int terrainHeight = static_cast<int>(CHUNK_BASE_HEIGHT + (height * amplitude));

            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                // setBlock(x, y, z, true);
                if (y < terrainHeight)
                    data.blocks[x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y)] = true;
            }
        }
    }

    int blockX = CHUNK_WIDTH / 2;
    int blockY = CHUNK_BASE_HEIGHT;
    int blockZ = CHUNK_WIDTH / 2;

    // setBlock(blockX, blockY, blockZ, true);

    return data;
}

MeshData ChunkManager::generateMesh(const BlockData &blockData, int chunkX, int chunkZ)
{
    MeshData data;

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

    auto isAir = [&](int x, int y, int z) -> bool
    {
        if (x < 0 || x >= CHUNK_WIDTH ||
            y < 0 || y >= CHUNK_HEIGHT ||
            z < 0 || z >= CHUNK_WIDTH)
            return true; // Out-of-bounds = air
        return !blockData.blocks[x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y)];
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
                    auto ao = GetVertexAOs(blockData, glm::ivec3(x, y, z), face.normal, chunkX, chunkZ);

                    unsigned int baseIndex = data.vertices.size() / 9; // 9 floats per vertex

                    auto pushVertex = [&](glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, float aoValue)
                    {
                        data.vertices.push_back(pos.x);
                        data.vertices.push_back(pos.y);
                        data.vertices.push_back(pos.z);
                        data.vertices.push_back(normal.x);
                        data.vertices.push_back(normal.y);
                        data.vertices.push_back(normal.z);
                        data.vertices.push_back(uv.x);
                        data.vertices.push_back(uv.y);
                        data.vertices.push_back(aoValue);
                    };

                    pushVertex(face.v0, face.normal, uv0, ao[0]);
                    pushVertex(face.v1, face.normal, uv1, ao[1]);
                    pushVertex(face.v2, face.normal, uv2, ao[2]);
                    pushVertex(face.v3, face.normal, uv3, ao[3]);

                    float diag1 = ao[0] + ao[2];
                    float diag2 = ao[1] + ao[3];

                    if (diag1 > diag2)
                        data.indices.insert(data.indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2,
                                                                 baseIndex, baseIndex + 2, baseIndex + 3});
                    else
                        data.indices.insert(data.indices.end(), {baseIndex + 1, baseIndex + 2, baseIndex + 3,
                                                                 baseIndex + 1, baseIndex + 3, baseIndex});
                }
            }
        }
    }

    return data;
}

int ChunkManager::getBlockIndex(int x, int y, int z) const
{
    return x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y);
}

bool ChunkManager::IsSolid(const BlockData &data, int x, int y, int z) const
{
    return blockAtSafe(data, x, y, z);
}

std::array<float, 4> ChunkManager::GetVertexAOs(
    const BlockData &localData,
    const Vector3i &blockPos,
    const Vector3i &faceNormal,
    int chunkX, int chunkZ)
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

        bool s1 = isBlockSolidGlobal(blockPos.x + p1.x, blockPos.y + p1.y, blockPos.z + p1.z, chunkX, chunkZ, localData);
        bool s2 = isBlockSolidGlobal(blockPos.x + p2.x, blockPos.y + p2.y, blockPos.z + p2.z, chunkX, chunkZ, localData);
        bool c = isBlockSolidGlobal(blockPos.x + p3.x, blockPos.y + p3.y, blockPos.z + p3.z, chunkX, chunkZ, localData);

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
        throw std::runtime_error("invalid face normal");
    }

    return neighbors[vertexIndex];
}
