#pragma once

#include "ChunkMesh.h"
#include "Math/Vector.h"
#include "Renderer/Texture.h"
#include "Util/Memory.h"
#include "World/Entity/Chunk.h"

#include <future>
#include <unordered_map>

class World;

struct PendingChunk
{
    std::future<ChunkMeshData> MeshDataFuture;
    Chunk *Chunk;
};

class ChunkManager
{
public:
    ChunkManager(World &world) : m_World(world) {}

    void Init();
    void Update(float delta);

private:
    void QueueChunk(Vector2i chunkPos);
    void PollPendingChunks();
    void UnloadChunks(int renderDistance, Vector2i playerChunkPos);

private:
    static constexpr int MAX_CHUNKS_PER_FRAME = 1;

    World &m_World;

    std::unordered_map<Vector2i, Chunk *> m_Chunks;
    std::unordered_map<Vector2i, PendingChunk> m_PendingChunks;
    std::vector<Vector2i> m_ChunkLoadQueue;

    Ref<TextureArray2D> m_ChunkTextureArray;
};