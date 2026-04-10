#include "ChunkManager.h"
#include "ChunkSnapshot.h"
#include "Engine.h"
#include "Math/Vector.h"
#include "Renderer/Texture.h"
#include "Util/Memory.h"
#include "World/Block/BlockRegistry.h"
#include "World/Entity/Component/Chunk/ChunkMeshGenerator.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Player.h"
#include "World/World.h"

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
    Player *player = m_World.GetPlayer();
    Vector3f playerPos = player->GetComponent<Transform>().Position;
    int renderDistance = player->GetRenderDistance();

    // Load chunks around the player (for simplicity, we just load a 3x3 area around the player)
    Vector2i playerChunkPos = Vector2i(static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE)), static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE)));
    for (int x = -renderDistance; x <= renderDistance; x++)
    {
        for (int z = -renderDistance; z <= renderDistance; z++)
        {
            Vector2i chunkPos = playerChunkPos + Vector2i(x, z);
            if (m_Chunks.find(chunkPos) == m_Chunks.end() && m_PendingChunks.find(chunkPos) == m_PendingChunks.end())
            {
                // Avoid duplicates in the queue
                if (std::find(m_ChunkLoadQueue.begin(), m_ChunkLoadQueue.end(), chunkPos) == m_ChunkLoadQueue.end())
                    m_ChunkLoadQueue.push_back(chunkPos);
            }
        }
    }

    int chunksQueued = 0;
    for (auto it = m_ChunkLoadQueue.begin(); it != m_ChunkLoadQueue.end() && chunksQueued < MAX_CHUNKS_PER_FRAME;)
    {
        QueueChunk(*it);
        it = m_ChunkLoadQueue.erase(it);
        chunksQueued++;
    }

    PollPendingChunks();
    UnloadChunks(renderDistance, playerChunkPos);
}

void ChunkManager::QueueChunk(Vector2i chunkPos)
{
    Chunk *chunk = new Chunk(chunkPos);
    ChunkSnapshot snapshot = chunk->CreateSnapshot();

    m_PendingChunks[chunkPos] = {
        std::async(std::launch::async, [chunk, snapshot = std::move(snapshot)]()
                   { return chunk->GetComponent<ChunkMeshGenerator>().GenerateMesh(snapshot); }),
        chunk};
}

void ChunkManager::PollPendingChunks()
{
    int uploadsThisFrame = 0;
    for (auto it = m_PendingChunks.begin(); it != m_PendingChunks.end() && uploadsThisFrame < MAX_CHUNKS_PER_FRAME;)
    {
        if (it->second.MeshDataFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            ChunkMeshData meshData = it->second.MeshDataFuture.get();
            it->second.Chunk->UploadMesh(meshData, m_ChunkTextureArray);
            m_World.RegisterEntity(it->second.Chunk);
            m_Chunks[it->first] = it->second.Chunk;
            it = m_PendingChunks.erase(it);
            uploadsThisFrame++;
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