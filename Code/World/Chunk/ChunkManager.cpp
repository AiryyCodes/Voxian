#include "ChunkManager.h"
#include "ChunkSnapshot.h"
#include "Engine.h"
#include "Math/Vector.h"
#include "Renderer/Renderer.h"
#include "Util/Memory.h"
#include "World/Block/BlockRegistry.h"
#include "World/Chunk/ChunkMesh.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include "World/Entity/Component/Chunk/ChunkMeshGenerator.h"
#include "World/Entity/Component/Chunk/ChunkMeshRenderer.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Player.h"
#include "World/World.h"

#include <algorithm>
#include <cstdint>
#include <future>
#include <utility>
#include <vector>

void ChunkManager::Init()
{
    BlockRegistry &blockRegistry = EngineContext::GetBlockRegistry();

    m_ChunkTextureArray = blockRegistry.GetTextureArray();

    m_Noise.Init();
}

void ChunkManager::Update(float delta)
{
    // Register all chunks from the last frame
    for (Ref<Chunk> chunk : m_ChunksReadyToRegister)
    {
        m_World.RegisterEntity(chunk);
    }
    m_ChunksReadyToRegister.clear();

    Player *player = m_World.GetPlayer();
    Vector3f playerPos = player->GetComponent<Transform>().Position;

    Vector2i playerChunkPos = Vector2i(
        static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE)),
        static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE)));

    // ONLY update the queue if the player moved to a new chunk
    if (playerChunkPos != m_LastPlayerChunkPos)
    {
        int renderDistance = player->GetRenderDistance();

        // Discover new chunks
        for (int x = -renderDistance; x <= renderDistance; x++)
        {
            for (int z = -renderDistance; z <= renderDistance; z++)
            {
                Vector2i chunkPos = playerChunkPos + Vector2i(x, z);
                if (m_Chunks.find(chunkPos) == m_Chunks.end() &&
                    m_PendingChunks.find(chunkPos) == m_PendingChunks.end() &&
                    m_InQueue.find(chunkPos) == m_InQueue.end())
                {
                    m_ChunkLoadQueue.push_back(chunkPos);
                    m_InQueue.insert(chunkPos);
                }
            }
        }

        UnloadChunks(renderDistance, playerChunkPos);

        m_LastPlayerChunkPos = playerChunkPos;
    }

    std::sort(m_ChunkLoadQueue.begin(), m_ChunkLoadQueue.end(),
              [playerChunkPos](const Vector2i &a, const Vector2i &b)
              {
                  // Distance squared for chunk A
                  int dxA = a.x - playerChunkPos.x;
                  int dzA = a.y - playerChunkPos.y;
                  int distSqA = dxA * dxA + dzA * dzA;

                  // Distance squared for chunk B
                  int dxB = b.x - playerChunkPos.x;
                  int dzB = b.y - playerChunkPos.y;
                  int distSqB = dxB * dxB + dzB * dzB;

                  return distSqA < distSqB; // Closer chunks come first
              });

    // Process a limited number of tasks from the queue per frame
    int chunksQueued = 0;
    while (!m_ChunkLoadQueue.empty() && chunksQueued < MAX_CHUNKS_PER_FRAME)
    {
        QueueChunk(m_ChunkLoadQueue.front());
        m_InQueue.erase(m_ChunkLoadQueue.front());
        m_ChunkLoadQueue.pop_front();
        chunksQueued++;
    }

    RebuildDirtyChunks();

    PollPendingChunks();

    UploadReadyChunks();

    if (!m_IsSpawnAreaReady)
    {
        if (m_Chunks.contains(m_SpawnChunkPos))
        {
            m_IsSpawnAreaReady = true;
            player->SetPhysicsEnabled(true);
            LOG_INFO("Spawn area ready! Spawning player...");
        }
        else
        {
            player->SetPhysicsEnabled(false);
        }
    }
}

void ChunkManager::Render(Renderer &renderer)
{
    for (auto &chunk : m_Chunks)
    {
        chunk.second->GetComponent<ChunkMeshRenderer>().RenderOpaque(renderer);
    }

    for (auto &chunk : m_Chunks)
    {
        chunk.second->GetComponent<ChunkMeshRenderer>().RenderTransparent(renderer);
    }
}

Ref<Chunk> ChunkManager::GetChunk(int x, int z)
{
    auto it = m_Chunks.find(Vector2i(x, z));
    return it != m_Chunks.end() ? it->second : nullptr;
}

bool ChunkManager::IsChunkLoaded(int x, int z)
{
    return m_Chunks.contains(Vector2i(x, z));
}

void ChunkManager::InitSpawnArea(Vector3f playerPos)
{
    m_SpawnChunkPos = Vector2i(
        static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE)),
        static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE)));

    // Add spawn chunk and 1-chunk radius to the very front of the queue
    for (int x = -1; x <= 1; x++)
    {
        for (int z = -1; z <= 1; z++)
        {
            Vector2i pos = m_SpawnChunkPos + Vector2i(x, z);
            if (!m_Chunks.contains(pos))
            {
                m_ChunkLoadQueue.push_front(pos);
                m_InQueue.insert(pos);
            }
        }
    }
};

void ChunkManager::QueueChunk(Vector2i chunkPos)
{
    auto chunk = CreateRef<Chunk>(chunkPos);

    auto future = m_ThreadPool.submit_task([this, chunk, noiseCopy = m_Noise]()
                                           {
        chunk->GetComponent<ChunkGenerator>().Generate(noiseCopy);

        ChunkSnapshot snapshot = chunk->CreateSnapshot();
        return chunk->GetComponent<ChunkMeshGenerator>().GenerateMesh(snapshot); });

    m_PendingChunks[chunkPos] = {std::move(future), chunk};
}

void ChunkManager::PollPendingChunks()
{
    for (auto it = m_PendingChunks.begin(); it != m_PendingChunks.end();)
    {
        PendingChunk &entry = it->second;

        if (entry.Future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            ++it;
            continue;
        }

        ReadyChunk ready;
        ready.Pos = it->first;
        ready.Mesh = entry.Future.get();
        ready.Chunk = entry.Chunk;

        m_ReadyQueue.push_back(std::move(ready));

        it = m_PendingChunks.erase(it);
    }
}

void ChunkManager::UploadReadyChunks()
{
    constexpr float MAX_MS = 4.0f;
    constexpr int MAX_CHUNKS = 1;

    auto start = std::chrono::high_resolution_clock::now();

    auto elapsed = [&]()
    {
        return std::chrono::duration<float, std::milli>(
                   std::chrono::high_resolution_clock::now() - start)
            .count();
    };

    int processed = 0;

    while (!m_ReadyQueue.empty())
    {
        if (processed >= MAX_CHUNKS || elapsed() >= MAX_MS)
            break;

        ReadyChunk ready = std::move(m_ReadyQueue.front());
        m_ReadyQueue.pop_front();

        ready.Chunk->UploadOpaqueMesh(ready.Mesh.Opaque, m_ChunkTextureArray);
        ready.Chunk->UploadTransparentMesh(ready.Mesh.Transparent, m_ChunkTextureArray);

        m_ChunksReadyToRegister.push_back(ready.Chunk);
        m_Chunks[ready.Pos] = ready.Chunk;

        processed++;
    }
}

void ChunkManager::UnloadChunks(int renderDistance, Vector2i playerChunkPos)
{
    auto outOfRange = [&](const Vector2i &p)
    {
        return std::abs(p.x - playerChunkPos.x) > renderDistance ||
               std::abs(p.y - playerChunkPos.y) > renderDistance;
    };

    // Remove loaded chunks
    std::vector<Vector2i> toUnload;

    for (auto &[pos, chunk] : m_Chunks)
    {
        if (outOfRange(pos))
            toUnload.push_back(pos);
    }

    for (const Vector2i &pos : toUnload)
    {
        m_World.DestroyEntity(m_Chunks[pos].get());
        m_Chunks.erase(pos);
    }

    // CCancel pending chunks
    for (auto it = m_PendingChunks.begin(); it != m_PendingChunks.end();)
    {
        const Vector2i &pos = it->first;

        if (outOfRange(pos))
        {
            it = m_PendingChunks.erase(it);
        }
        else
        {
            ++it;
        }
    }

    m_ChunkLoadQueue.erase(
        std::remove_if(m_ChunkLoadQueue.begin(), m_ChunkLoadQueue.end(),
                       [&](const Vector2i &pos)
                       {
                           return outOfRange(pos);
                       }),
        m_ChunkLoadQueue.end());

    m_ReadyQueue.erase(
        std::remove_if(m_ReadyQueue.begin(), m_ReadyQueue.end(),
                       [&](const ReadyChunk &c)
                       {
                           return outOfRange(c.Pos);
                       }),
        m_ReadyQueue.end());

    m_ChunksReadyToRegister.erase(
        std::remove_if(m_ChunksReadyToRegister.begin(), m_ChunksReadyToRegister.end(),
                       [&](const Ref<Chunk> &c)
                       {
                           auto pos = c->GetPosition();
                           return outOfRange(pos);
                       }),
        m_ChunksReadyToRegister.end());
}

const Block *ChunkManager::GetBlock(int x, int y, int z) const
{
    int chunkX = (x >= 0) ? (x / CHUNK_SIZE) : ((x - CHUNK_SIZE + 1) / CHUNK_SIZE);
    int chunkZ = (z >= 0) ? (z / CHUNK_SIZE) : ((z - CHUNK_SIZE + 1) / CHUNK_SIZE);
    auto it = m_Chunks.find(Vector2i(chunkX, chunkZ));
    if (it == m_Chunks.end())
        return nullptr;

    Ref<Chunk> chunk = it->second;

    int localX = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int localZ = ((z % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

    uint16_t blockId = chunk->GetBlock(localX, y, localZ);
    return EngineContext::GetBlockRegistry().GetBlockByIndex(blockId);
}

void ChunkManager::SetBlock(int x, int y, int z, uint16_t id)
{
    int chunkX = (x >= 0) ? (x / CHUNK_SIZE) : ((x - CHUNK_SIZE + 1) / CHUNK_SIZE);
    int chunkZ = (z >= 0) ? (z / CHUNK_SIZE) : ((z - CHUNK_SIZE + 1) / CHUNK_SIZE);

    auto it = m_Chunks.find(Vector2i(chunkX, chunkZ));
    if (it == m_Chunks.end())
        return;

    int localX = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int localZ = ((z % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

    it->second->SetBlock(localX, y, localZ, id);
    MarkChunkDirty(Vector2i(chunkX, chunkZ));

    // If block is on a chunk border, dirty the neighbor too
    if (localX == 0)
        MarkChunkDirty(Vector2i(chunkX - 1, chunkZ));
    if (localX == CHUNK_SIZE - 1)
        MarkChunkDirty(Vector2i(chunkX + 1, chunkZ));
    if (localZ == 0)
        MarkChunkDirty(Vector2i(chunkX, chunkZ - 1));
    if (localZ == CHUNK_SIZE - 1)
        MarkChunkDirty(Vector2i(chunkX, chunkZ + 1));
}

void ChunkManager::MarkChunkDirty(Vector2i chunkPos)
{
    if (m_Chunks.contains(chunkPos) && !m_PendingChunks.contains(chunkPos))
        m_DirtyChunks.insert(chunkPos);
}

void ChunkManager::RebuildDirtyChunks()
{
    for (const Vector2i &pos : m_DirtyChunks)
    {
        auto it = m_Chunks.find(pos);
        if (it == m_Chunks.end())
            continue;

        Ref<Chunk> chunk = it->second;
        ChunkSnapshot snapshot = CreateSnapshotWithNeighbors(chunk);

        auto future = m_ThreadPool.submit_task(
            [chunk, snapshot = std::move(snapshot)]() mutable
            {
                return chunk->GetComponent<ChunkMeshGenerator>().GenerateMesh(snapshot);
            });

        m_PendingChunks[pos] = {std::move(future), chunk};
    }

    m_DirtyChunks.clear();
}

ChunkSnapshot ChunkManager::CreateSnapshotWithNeighbors(Ref<Chunk> chunk)
{
    ChunkSnapshot snapshot = chunk->CreateSnapshot();

    Vector2i pos = chunk->GetPosition();

    // Helper to get a block id from a neighbor, or 0 if not loaded
    auto getNeighborBlock = [&](int nx, int ny, int nz, Vector2i neighborChunkPos) -> uint16_t
    {
        auto it = m_Chunks.find(neighborChunkPos);
        if (it == m_Chunks.end())
            return 0;
        return it->second->GetBlock(nx, ny, nz);
    };

    // Fill -X border (localX = 0 in padding)
    auto leftChunk = m_Chunks.find(Vector2i(pos.x - 1, pos.y));
    if (leftChunk != m_Chunks.end())
        for (int z = 0; z < CHUNK_SIZE; z++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                snapshot.SetBlock(0, y + 1, z + 1, leftChunk->second->GetBlock(CHUNK_SIZE - 1, y, z));

    // Fill +X border
    auto rightChunk = m_Chunks.find(Vector2i(pos.x + 1, pos.y));
    if (rightChunk != m_Chunks.end())
        for (int z = 0; z < CHUNK_SIZE; z++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                snapshot.SetBlock(PADDED_CHUNK_SIZE - 1, y + 1, z + 1, rightChunk->second->GetBlock(0, y, z));

    // Fill -Z border
    auto backChunk = m_Chunks.find(Vector2i(pos.x, pos.y - 1));
    if (backChunk != m_Chunks.end())
        for (int x = 0; x < CHUNK_SIZE; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                snapshot.SetBlock(x + 1, y + 1, 0, backChunk->second->GetBlock(x, y, CHUNK_SIZE - 1));

    // Fill +Z border
    auto frontChunk = m_Chunks.find(Vector2i(pos.x, pos.y + 1));
    if (frontChunk != m_Chunks.end())
        for (int x = 0; x < CHUNK_SIZE; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                snapshot.SetBlock(x + 1, y + 1, PADDED_CHUNK_SIZE - 1, frontChunk->second->GetBlock(x, y, 0));

    return snapshot;
}

void ChunkManager::UploadMesh(ChunkMeshData &meshData)
{
}