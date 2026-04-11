#include "ChunkManager.h"
#include "ChunkSnapshot.h"
#include "Engine.h"
#include "Math/Vector.h"
#include "Renderer/Texture.h"
#include "Util/Memory.h"
#include "World/Block/BlockRegistry.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include "World/Entity/Component/Chunk/ChunkMeshGenerator.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Player.h"
#include "World/World.h"

#include <cstdint>
#include <future>
#include <utility>
#include <vector>

void ChunkManager::Init()
{
    BlockRegistry &blockRegistry = EngineContext::GetBlockRegistry();
    std::vector<std::string> textures = blockRegistry.GetAllBlockTextures();

    m_ChunkTextureArray = CreateRef<TextureArray2D>();
    m_ChunkTextureArray->Init(16, 16, textures.size(), textures, GL_RGB);

    m_Noise.Init();
}

void ChunkManager::Update(float delta)
{
    // Register all chunks from the last frame
    for (Chunk *chunk : m_ChunksReadyToRegister)
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

        // 1. Discover new chunks
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

    // Process a limited number of tasks from the queue per frame
    int chunksQueued = 0;
    while (!m_ChunkLoadQueue.empty() && chunksQueued < MAX_CHUNKS_PER_FRAME)
    {
        QueueChunk(m_ChunkLoadQueue.front());
        m_InQueue.erase(m_ChunkLoadQueue.front());
        m_ChunkLoadQueue.pop_front();
        chunksQueued++;
    }

    PollPendingChunks();
}

void ChunkManager::QueueChunk(Vector2i chunkPos)
{
    Chunk *chunk = new Chunk(chunkPos);

    auto future = m_ThreadPool.submit_task([this, chunk, noiseCopy = m_Noise]()
                                           {
        chunk->GetComponent<ChunkGenerator>().Generate(noiseCopy);

        ChunkSnapshot snapshot = chunk->CreateSnapshot();
        return chunk->GetComponent<ChunkMeshGenerator>().GenerateMesh(snapshot); });

    m_PendingChunks[chunkPos] = {std::move(future), chunk};
}

void ChunkManager::PollPendingChunks()
{
    constexpr float MAX_UPLOAD_MS = 2.0f;
    auto start = std::chrono::high_resolution_clock::now();

    for (auto it = m_PendingChunks.begin(); it != m_PendingChunks.end();)
    {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<float, std::milli>(now - start).count() >= MAX_UPLOAD_MS)
            break;

        if (it->second.MeshDataFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            ChunkMeshData meshData = it->second.MeshDataFuture.get();
            it->second.Chunk->UploadMesh(meshData, m_ChunkTextureArray);

            m_ChunksReadyToRegister.push_back(it->second.Chunk);

            m_Chunks[it->first] = it->second.Chunk;
            it = m_PendingChunks.erase(it);
        }
        else
            ++it;
    }
}

void ChunkManager::UnloadChunks(int renderDistance, Vector2i playerChunkPos)
{
    std::vector<Vector2i> chunksToUnload;
    for (const auto &entry : m_Chunks)
    {
        Vector2i chunkPos = entry.first;
        if (std::abs(chunkPos.x - playerChunkPos.x) > renderDistance ||
            std::abs(chunkPos.y - playerChunkPos.y) > renderDistance)
            chunksToUnload.push_back(chunkPos);
    }
    for (const auto &pos : chunksToUnload)
    {
        m_World.DestroyEntity(m_Chunks[pos]);
        m_Chunks.erase(pos);
    }

    // Also cancel pending chunks that have gone out of range
    for (auto it = m_PendingChunks.begin(); it != m_PendingChunks.end();)
    {
        Vector2i chunkPos = it->first;
        if (std::abs(chunkPos.x - playerChunkPos.x) > renderDistance ||
            std::abs(chunkPos.y - playerChunkPos.y) > renderDistance)
        {
            if (it->second.MeshDataFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                m_World.DestroyEntity(it->second.Chunk);
                it = m_PendingChunks.erase(it);
            }
            else
                ++it; // still generating, check again next frame
        }
        else
            ++it;
    }

    m_ChunkLoadQueue.erase(
        std::remove_if(m_ChunkLoadQueue.begin(), m_ChunkLoadQueue.end(), [&](const Vector2i &pos)
                       { return std::abs(pos.x - playerChunkPos.x) > renderDistance ||
                                std::abs(pos.y - playerChunkPos.y) > renderDistance; }),
        m_ChunkLoadQueue.end());
}

const Block *ChunkManager::GetBlock(int x, int y, int z) const
{
    int chunkX = (x >= 0) ? (x / CHUNK_SIZE) : ((x - CHUNK_SIZE + 1) / CHUNK_SIZE);
    int chunkZ = (z >= 0) ? (z / CHUNK_SIZE) : ((z - CHUNK_SIZE + 1) / CHUNK_SIZE);
    auto it = m_Chunks.find(Vector2i(chunkX, chunkZ));
    if (it == m_Chunks.end())
        return nullptr;

    Chunk *chunk = it->second;

    int localX = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int localZ = ((z % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

    uint16_t blockId = chunk->GetBlock(localX, y, localZ);
    return EngineContext::GetBlockRegistry().GetBlockByIndex(blockId);
}
