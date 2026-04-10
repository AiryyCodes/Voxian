#pragma once

#include "ChunkMesh.h"
#include "Math/Vector.h"
#include "Renderer/Texture.h"
#include "Util/Memory.h"
#include "World/Chunk/Terrain/TerrainNoise.h"
#include "World/Entity/Chunk.h"

#include <deque>
#include <future>
#include <unordered_map>
#include <BS_thread_pool.hpp>
#include <unordered_set>

class World;

struct PendingChunk
{
    std::future<ChunkMeshData> MeshDataFuture;
    class Chunk *Chunk;
};

class ChunkManager
{
public:
    ChunkManager(World &world) : m_World(world) {}

    void Init();
    void Update(float delta);

    const TerrainNoise &GetTerrainNoise() const { return m_Noise; }

private:
    void QueueChunk(Vector2i chunkPos);
    void PollPendingChunks();
    void UnloadChunks(int renderDistance, Vector2i playerChunkPos);

private:
    static constexpr int MAX_CHUNKS_PER_FRAME = 4;

    TerrainNoise m_Noise;

    World &m_World;

    std::unordered_map<Vector2i, Chunk *> m_Chunks;
    std::unordered_map<Vector2i, PendingChunk> m_PendingChunks;
    std::deque<Vector2i> m_ChunkLoadQueue;

    Vector2i m_LastPlayerChunkPos = {INT_MAX, INT_MAX};
    std::unordered_set<Vector2i> m_InQueue;
    std::vector<Chunk *> m_ChunksReadyToRegister;

    Ref<TextureArray2D> m_ChunkTextureArray;

    BS::thread_pool<> m_ThreadPool;
};
