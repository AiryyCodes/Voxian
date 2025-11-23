
#include "World/ChunkManager.h"
#include "FastNoiseLite.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "Logger.h"
#include "Math/Vector.h"
#include "Memory.h"
#include "Queue.h"
#include "World/Block.h"
#include "World/Chunk.h"
#include "World/Structure.h"

#include <algorithm>
#include <memory>
#include <stdexcept>

void ChunkManager::RequestChunk(int x, int z)
{
    Vector2i key{x, z};
    std::shared_ptr<Chunk> chunk;

    {
        std::scoped_lock lock(m_ChunkMutex);

        // If chunk exists and isn't empty, skip
        auto it = m_Chunks.find(key);
        if (it != m_Chunks.end())
        {
            if (it->second->GetState() != Chunk::State::Empty)
                return;
            chunk = it->second;
        }
        else
        {
            // Create new chunk
            chunk = std::make_shared<Chunk>(this, x, z);
            m_Chunks[key] = chunk;
        }
        m_RequestedChunks.insert(key);
        chunk->SetState(Chunk::State::Generating);
    }

    std::weak_ptr<Chunk> weakChunk = chunk;

    // Async block generation
    m_ThreadPool.post([this, weakChunk]()
                      {
        if (auto chunk = weakChunk.lock())
        {
            BlockData blockData = GenerateBlocks(chunk->m_Position.x, chunk->m_Position.y);

            Vector2i chunkPos = chunk->GetPosition();
            ApplyStructures(chunkPos.x, chunkPos.y, blockData);

            // Push block result to main thread queue
            m_BlockQueue.push({
                .callback = [weakChunk](BlockData &data)
                {
                    if (auto chunk = weakChunk.lock())
                    {
                        chunk->SetBlockData(data);
                        chunk->SetState(Chunk::State::BlocksReady);
                    }
                },
                .result = std::move(blockData)
            });
        } });
}

void ChunkManager::UpdatePlayerPosition(const Vector3 &pos)
{
    m_PlayerPosition = pos;
    Vector2i center{
        int(std::floor(pos.x / CHUNK_WIDTH)),
        int(std::floor(pos.z / CHUNK_WIDTH))};

    // Calculate desired chunks
    std::vector<std::pair<int, Vector2i>> desired;
    for (int dz = -m_ViewDistance; dz <= m_ViewDistance; dz++)
    {
        for (int dx = -m_ViewDistance; dx <= m_ViewDistance; dx++)
        {
            Vector2i c{center.x + dx, center.y + dz};
            int dist2 = dx * dx + dz * dz;
            desired.emplace_back(dist2, c);
        }
    }

    std::sort(desired.begin(), desired.end(),
              [](auto &a, auto &b)
              { return a.first < b.first; });

    // Mark unloads
    {
        std::scoped_lock lock(m_ChunkMutex);
        for (auto &[pos, chunk] : m_Chunks)
        {
            bool keep = false;
            for (auto &d : desired)
                if (d.second == pos)
                    keep = true;

            chunk->m_ShouldUnload = !keep;
        }
    }

    // -------------------------
    // Request a FEW missing chunks
    // -------------------------
    int MAX_REQUESTS_PER_FRAME = 2;
    int made = 0;

    for (auto &[dist, c] : desired)
    {
        if (made >= MAX_REQUESTS_PER_FRAME)
            break;

        // Already exists?
        if (m_Chunks.find(c) != m_Chunks.end())
            continue;

        // Already requested?
        if (m_RequestedChunks.find(c) != m_RequestedChunks.end())
            continue;

        // Request it now
        RequestChunk(c.x, c.y);
        made++;
    }
}

void ChunkManager::Update(const Shader &shader)
{
    int px = static_cast<int>(std::floor(m_PlayerPosition.x / CHUNK_WIDTH));
    int pz = static_cast<int>(std::floor(m_PlayerPosition.z / CHUNK_WIDTH));
    Vector2i playerChunk{px, pz};

    // Apply block results
    ThreadTask<BlockData> blockTask;
    while (m_BlockQueue.pop(blockTask))
    {
        blockTask.callback(blockTask.result);
    }

    // Apply mesh results
    ThreadTask<MeshData> meshTask;
    while (m_MeshQueue.pop(meshTask))
    {
        meshTask.callback(meshTask.result);
    }

    // Collect chunks that need mesh rebuild
    std::vector<std::shared_ptr<Chunk>> dirtyChunks;
    {
        std::scoped_lock lock(m_ChunkMutex);
        for (auto &[pos, chunk] : m_Chunks)
        {
            Chunk::State state = chunk->GetState();

            // Only rebuild if blocks are ready but mesh not yet generated
            if ((state == Chunk::State::BlocksReady || state == Chunk::State::MeshReady) && chunk->m_NeedsRebuild)
            {
                dirtyChunks.push_back(chunk);
            }
        }
    }

    // Sort by distance to player so closest chunks rebuild first
    std::sort(dirtyChunks.begin(), dirtyChunks.end(),
              [&](const std::shared_ptr<Chunk> &a, const std::shared_ptr<Chunk> &b)
              {
                  int dxA = a->GetPosition().x - playerChunk.x;
                  int dzA = a->GetPosition().y - playerChunk.y;
                  int dxB = b->GetPosition().x - playerChunk.x;
                  int dzB = b->GetPosition().y - playerChunk.y;
                  return (dxA * dxA + dzA * dzA) < (dxB * dxB + dzB * dzB);
              });

    // Post mesh rebuild tasks
    int rebuildCount = 0;
    for (auto &chunk : dirtyChunks)
    {
        if (rebuildCount >= m_MaxRebuildsPerFrame)
            break;

        chunk->m_NeedsRebuild = false;

        m_ThreadPool.post([this, chunk]()
                          {
            // Only generate mesh if blocks are ready
            if (chunk->GetState() == Chunk::State::BlocksReady || chunk->GetState() == Chunk::State::MeshReady)
            {
                MeshData newMesh = GenerateMesh(chunk->m_Blocks, chunk->GetPosition().x, chunk->GetPosition().y);

                // Push result to mesh queue
                m_MeshQueue.push({.callback = [chunk](MeshData &mesh)
                {
                    chunk->SetMeshData(mesh);
                    chunk->SetState(Chunk::State::MeshReady); // transition to MeshReady
                },
                .result = std::move(newMesh)});
            } });

        rebuildCount++;
    }

    // Upload meshes, draw, unload
    const int renderDistance = m_ViewDistance;

    std::vector<std::shared_ptr<Chunk>> opaqueChunks;
    std::vector<std::shared_ptr<Chunk>> transparentChunks;

    {
        std::scoped_lock lock(m_ChunkMutex);

        for (auto it = m_Chunks.begin(); it != m_Chunks.end();)
        {
            auto &chunk = it->second;
            int dx = chunk->GetPosition().x - playerChunk.x;
            int dz = chunk->GetPosition().y - playerChunk.y;
            int dist2 = dx * dx + dz * dz;

            Chunk::State state = chunk->GetState();

            // Upload mesh if ready
            if (state == Chunk::State::MeshReady)
            {
                chunk->UploadMeshToGPU();
                chunk->SetState(Chunk::State::Done);
            }

            // Collect chunks in range
            if (state == Chunk::State::Done && dist2 <= renderDistance * renderDistance)
            {
                opaqueChunks.push_back(chunk); // opaque and transparent will be separated in Draw()
            }

            // Unload far-away chunks
            if (chunk->m_ShouldUnload)
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
    }

    // Separate opaque and transparent
    for (auto &chunk : opaqueChunks)
    {
        if (!chunk->m_Mesh.Transparent.Indices.empty())
            transparentChunks.push_back(chunk);
    }

    // Draw opaque first
    for (auto &chunk : opaqueChunks)
        chunk->DrawOpaque(shader);

    // Sort transparent chunks back-to-front
    std::sort(transparentChunks.begin(), transparentChunks.end(),
              [&](const std::shared_ptr<Chunk> &a, const std::shared_ptr<Chunk> &b)
              {
                  float da = glm::length(m_PlayerPosition - Vector3(a->GetPosition().x * CHUNK_WIDTH, 0, a->GetPosition().y * CHUNK_WIDTH));
                  float db = glm::length(m_PlayerPosition - Vector3(b->GetPosition().x * CHUNK_WIDTH, 0, b->GetPosition().y * CHUNK_WIDTH));
                  return da > db; // farthest first
              });

    // Draw transparent
    for (auto &chunk : transparentChunks)
        chunk->DrawTransparent(shader);
}

std::shared_ptr<Chunk> ChunkManager::GetChunk(int x, int z)
{
    std::scoped_lock lock(m_ChunkMutex);
    auto it = m_Chunks.find(Vector2i(x, z));
    return it != m_Chunks.end() ? it->second : nullptr;
}

const BlockState &ChunkManager::GetBlock(
    int x, int y, int z,
    int chunkX, int chunkZ,
    const BlockData &localData)
{
    // Height check (padded supports y=-1 and y=H)
    if (y < -1 || y > CHUNK_HEIGHT)
        return g_BlockRegistry.Get(BLOCK_AIR);

    // Convert to padded coordinates
    int px = x + 1;
    int py = y + 1;
    int pz = z + 1;

    // Padded bounds:
    // px: 0 .. W+1
    // py: 0 .. H+1
    // pz: 0 .. W+1
    if (px < 0 || px >= BlockData::PW ||
        py < 0 || py >= BlockData::PH ||
        pz < 0 || pz >= BlockData::PW)
    {
        return g_BlockRegistry.Get(BLOCK_AIR);
    }

    // Direct sample from padded chunk
    return localData.Get(px, py, pz);
}

BlockData ChunkManager::GenerateBlocks(int chunkX, int chunkZ)
{
    GenerateTreesForChunk(chunkX, chunkZ);

    BlockData data;

    const int W = CHUNK_WIDTH;
    const int H = CHUNK_HEIGHT;

    const int PW = W + 2;
    const int PH = H + 2;

    const float amplitude = 20.0f;
    const int minDirt = 1;
    const int maxDirt = 1;
    const int seaLevel = CHUNK_BASE_HEIGHT;

    for (int px = 0; px < PW; ++px)
    {
        for (int pz = 0; pz < PW; ++pz)
        {
            int wx = chunkX * W + (px - 1);
            int wz = chunkZ * W + (pz - 1);

            // Base terrain noise
            float hNoise = m_Noise.GetNoise(float(wx), float(wz));
            int terrainHeight = int(CHUNK_BASE_HEIGHT + hNoise * amplitude);

            if (terrainHeight < 0)
                terrainHeight = 0;
            if (terrainHeight >= H)
                terrainHeight = H - 1;

            // Optional: vary dirt thickness per column
            int dirtThickness = minDirt + (int)(m_Noise.GetNoise(float(wx + 1000), float(wz + 1000)) * (maxDirt - minDirt));
            if (dirtThickness < minDirt)
                dirtThickness = minDirt;
            if (dirtThickness > maxDirt)
                dirtThickness = maxDirt;

            int stoneHeight = terrainHeight - dirtThickness;
            if (stoneHeight < 1)
                stoneHeight = 1; // always leave at least 1 stone block

            for (int py = 0; py < PH; ++py)
            {
                int wy = py - 1;
                uint16_t id = BLOCK_AIR;

                if (wy <= 0)
                {
                    id = BLOCK_BEDROCK;
                }
                else if (wy > terrainHeight)
                {
                    id = BLOCK_AIR;
                }
                else if (wy == terrainHeight)
                {
                    id = BLOCK_GRASS; // top block is always grass
                }
                else if (wy >= terrainHeight - dirtThickness)
                {
                    id = BLOCK_DIRT; // dirt layer below grass
                }
                else
                {
                    id = BLOCK_STONE; // everything below dirt is stone
                }

                data.SetID(px, py, pz, id);
            }
        }
    }

    return data;
}

void ChunkManager::GenerateTreesForChunk(int cx, int cz)
{
    const int W = CHUNK_WIDTH;
    Vector2i key{cx, cz};
    auto &list = m_Trees[key]; // creates entry if missing

    // Decide tree count based on noise
    float noiseVal = m_Noise.GetNoise(float(cx * 10), float(cz * 10));
    int treeCount = int((noiseVal + 1.0f) * 2.0f); // 0..4 trees per chunk

    for (int i = 0; i < treeCount; ++i)
    {
        // Pick deterministic base coordinates in the chunk
        float fx = m_Noise.GetNoise(float(cx * 100 + i), float(cz * 100 + i));
        float fz = m_Noise.GetNoise(float(cx * 200 + i), float(cz * 200 + i));

        int wx = cx * W + int((fx + 1.0f) * 0.5f * (W - 4)) + 2;
        int wz = cz * W + int((fz + 1.0f) * 0.5f * (W - 4)) + 2;

        // Determine terrain height using noise
        int wy = int(CHUNK_BASE_HEIGHT + m_Noise.GetNoise(float(wx), float(wz)) * 20.0f);
        if (wy < 1)
            wy = 1;
        if (wy >= CHUNK_HEIGHT - 1)
            wy = CHUNK_HEIGHT - 2;

        Tree tree;
        tree.BasePos = Vector3i(wx, wy, wz);
        tree.Blocks = GenerateTreeBlocks(tree.BasePos);

        list.push_back(tree);
    }
}

MeshData ChunkManager::GenerateMesh(const BlockData &blockData, int chunkX, int chunkZ)
{
    MeshData data;

    Vector3f p000(0, 0, 0), p001(0, 0, 1), p010(0, 1, 0), p011(0, 1, 1);
    Vector3f p100(1, 0, 0), p101(1, 0, 1), p110(1, 1, 0), p111(1, 1, 1);
    Vector2f uv0(0, 1), uv1(1, 1), uv2(1, 0), uv3(0, 0);

    for (int x = 0; x < CHUNK_WIDTH; ++x)
    {
        for (int y = 0; y < CHUNK_HEIGHT; ++y)
        {
            for (int z = 0; z < CHUNK_WIDTH; ++z)
            {
                const BlockState &state = GetBlock(x, y, z, chunkX, chunkZ, blockData);
                if (state.IsAir())
                    continue;

                bool isTransparent = state.IsTransparent();
                int blockId = state.GetId();
                Vector3 blockPos(x, y, z);

                struct Face
                {
                    Vector3f v0, v1, v2, v3;
                    Vector3i normal;
                };

                Face faces[6] = {
                    {blockPos + p100, blockPos + p101, blockPos + p111, blockPos + p110, Vector3i(1, 0, 0)},  // +X
                    {blockPos + p001, blockPos + p000, blockPos + p010, blockPos + p011, Vector3i(-1, 0, 0)}, // -X
                    {blockPos + p010, blockPos + p110, blockPos + p111, blockPos + p011, Vector3i(0, 1, 0)},  // +Y
                    {blockPos + p001, blockPos + p101, blockPos + p100, blockPos + p000, Vector3i(0, -1, 0)}, // -Y
                    {blockPos + p101, blockPos + p001, blockPos + p011, blockPos + p111, Vector3i(0, 0, 1)},  // +Z
                    {blockPos + p000, blockPos + p100, blockPos + p110, blockPos + p010, Vector3i(0, 0, -1)}, // -Z
                };

                for (auto &face : faces)
                {
                    int nx = x + face.normal.x;
                    int ny = y + face.normal.y;
                    int nz = z + face.normal.z;

                    const BlockState &adj = GetBlock(nx, ny, nz, chunkX, chunkZ, blockData);

                    bool skip = false;

                    if (!isTransparent)
                    {
                        // opaque: skip if neighbor is opaque
                        skip = (!adj.IsAir() && !adj.IsTransparent());
                    }

                    if (isTransparent && face.normal.y != 0)
                    {
                        skip = false; // always draw top/bottom
                    }

                    if (skip)
                        continue;

                    auto &destMesh = isTransparent ? data.Transparent : data.Opaque;
                    auto &verts = destMesh.Vertices;
                    auto &inds = destMesh.Indices;

                    unsigned int baseIndex = verts.size();

                    auto ao = GetVertexAOs(blockData, Vector3i(x, y, z), Vector3i(face.normal), Vector2i(chunkX, chunkZ));

                    auto push = [&](Vector3f pos, Vector3i normal, Vector2f uv, float aoVal)
                    {
                        Ref<TextureArray2D> texture = g_BlockRegistry.GetTexture();
                        if (!texture)
                            return;

                        BlockVertex v;
                        v.Position = pos;
                        v.Normal = normal;
                        v.UV = uv;
                        v.TextureSize = Vector2i(texture->GetWidth(blockId * 6),
                                                 texture->GetHeight(blockId * 6));
                        v.Layer = blockId * 6;

                        aoVal = isTransparent ? 1.0f : aoVal;
                        v.AO = aoVal;

                        verts.push_back(v);
                    };

                    // Push vertices
                    push(face.v0, face.normal, uv0, ao[0]);
                    push(face.v1, face.normal, uv1, ao[1]);
                    push(face.v2, face.normal, uv2, ao[2]);
                    push(face.v3, face.normal, uv3, ao[3]);

                    float diag1 = ao[0] + ao[2];
                    float diag2 = ao[1] + ao[3];

                    if (diag1 > diag2)
                    {
                        inds.insert(inds.end(),
                                    {baseIndex, baseIndex + 1, baseIndex + 2,
                                     baseIndex, baseIndex + 2, baseIndex + 3});
                    }
                    else
                    {
                        inds.insert(inds.end(),
                                    {baseIndex + 1, baseIndex + 2, baseIndex + 3,
                                     baseIndex + 1, baseIndex + 3, baseIndex});
                    }
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

        bool s1 = GetBlock(blockPos.x + p1.x, blockPos.y + p1.y, blockPos.z + p1.z, chunkPos.x, chunkPos.y, localData).IsSolid();
        bool s2 = GetBlock(blockPos.x + p2.x, blockPos.y + p2.y, blockPos.z + p2.z, chunkPos.x, chunkPos.y, localData).IsSolid();
        bool c = GetBlock(blockPos.x + p3.x, blockPos.y + p3.y, blockPos.z + p3.z, chunkPos.x, chunkPos.y, localData).IsSolid();

        ao[i] = getOcclusion(s1, s2, c);
    }

    if (faceNormal == Vector3i(0, -1, 0))
        std::swap(ao[0], ao[2]);

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

void ChunkManager::PlaceTree(const Tree &tree, BlockData &data, const Vector2i &chunkPos)
{
    const int W = CHUNK_WIDTH;
    const int H = CHUNK_HEIGHT;
    const int PW = W + 2;
    const int PH = H + 2;

    // Iterate all tree blocks
    for (const auto &tb : tree.Blocks)
    {
        // World coordinates of the tree block
        int wx = tree.BasePos.x + tb.RelativePos.x;
        int wy = tree.BasePos.y + tb.RelativePos.y;
        int wz = tree.BasePos.z + tb.RelativePos.z;

        // Determine if this block belongs to this chunk
        int cx = (wx >= 0) ? wx / W : (wx - W + 1) / W;
        int cz = (wz >= 0) ? wz / W : (wz - W + 1) / W;

        if (cx != chunkPos.x || cz != chunkPos.y)
            continue; // Not in this chunk

        // Convert world coords → local padded coords
        int lx = (wx - chunkPos.x * W) + 1;
        int ly = wy + 1; // padded y
        int lz = (wz - chunkPos.y * W) + 1;

        if (lx < 0 || lx >= PW || ly < 0 || ly >= PH || lz < 0 || lz >= PW)
            continue; // Outside padded range

        data.SetID(lx, ly, lz, tb.BlockId);
    }
}

bool ChunkManager::TreeIntersectsChunk(const Tree &tree, const Vector2i &chunkPos) const
{
    const int W = CHUNK_WIDTH;

    // Compute the bounding box of the tree in world coordinates
    int minX = tree.BasePos.x;
    int maxX = tree.BasePos.x;
    int minZ = tree.BasePos.z;
    int maxZ = tree.BasePos.z;

    for (const auto &tb : tree.Blocks)
    {
        int wx = tree.BasePos.x + tb.RelativePos.x;
        int wz = tree.BasePos.z + tb.RelativePos.z;

        if (wx < minX)
            minX = wx;
        if (wx > maxX)
            maxX = wx;
        if (wz < minZ)
            minZ = wz;
        if (wz > maxZ)
            maxZ = wz;
    }

    // Chunk bounds in world coordinates
    int chunkMinX = chunkPos.x * W;
    int chunkMaxX = chunkMinX + W - 1;
    int chunkMinZ = chunkPos.y * W;
    int chunkMaxZ = chunkMinZ + W - 1;

    // Check if the tree bounding box intersects the chunk
    bool intersectsX = (maxX >= chunkMinX) && (minX <= chunkMaxX);
    bool intersectsZ = (maxZ >= chunkMinZ) && (minZ <= chunkMaxZ);

    return intersectsX && intersectsZ;
}

std::vector<TreeBlock> ChunkManager::GenerateTreeBlocks(const Vector3i &base)
{
    std::vector<TreeBlock> blocks;

    const int trunkHeight = 5;
    const int leafStart = trunkHeight - 1;
    const int leafRadius = 2;
    const int leafHeight = 4;

    // Trunk
    for (int y = 0; y < trunkHeight; ++y)
    {
        blocks.push_back({Vector3i(0, y, 0), BLOCK_WOOD_LOG});
    }

    // Leaves (simple cube)
    for (int x = -leafRadius; x <= leafRadius; ++x)
    {
        for (int y = leafStart; y <= leafStart + leafHeight; ++y)
        {
            for (int z = -leafRadius; z <= leafRadius; ++z)
            {
                if (x == 0 && y < trunkHeight && z == 0)
                    continue; // skip trunk space
                blocks.push_back({Vector3i(x, y, z), BLOCK_LEAVES});
            }
        }
    }

    return blocks;
}

void ChunkManager::ApplyStructures(int cx, int cz, BlockData &data)
{
    const Vector2i currentChunk{cx, cz};

    // Check this chunk and the 8 neighbors
    for (int dx = -1; dx <= 1; dx++)
        for (int dz = -1; dz <= 1; dz++)
        {
            Vector2i key{cx + dx, cz + dz};

            auto it = m_Trees.find(key);
            if (it == m_Trees.end())
                continue;

            for (const Tree &tree : it->second)
            {
                if (TreeIntersectsChunk(tree, currentChunk))
                {
                    PlaceTree(tree, data, currentChunk);
                }
            }
        }
}
