#pragma once

#include "ChunkMesh.h"
#include "Math/Vector.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture.h"
#include "Util/Memory.h"
#include "World/Chunk/Terrain/TerrainNoise.h"
#include "World/Entity/Chunk.h"

#include <cstdint>
#include <deque>
#include <future>
#include <unordered_map>
#include <BS_thread_pool.hpp>
#include <unordered_set>

class World;

enum class ChunkStage
{
    Generating,
    Ready,
    Uploaded
};

struct PendingChunk
{
    std::future<ChunkMeshGroup> Future;
    Ref<Chunk> Chunk;
};

struct ReadyChunk
{
    Vector2i Pos;
    ChunkMeshGroup Mesh;
    Ref<Chunk> Chunk;
};

class ChunkManager
{
public:
    ChunkManager(World &world) : m_World(world) {}

    void Init();
    void Update(float delta);
    void Render(Renderer &renderer);

    Ref<Chunk> GetChunk(int x, int z);
    bool IsChunkLoaded(int x, int z);

    const Block *GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, uint16_t id);

    const TerrainNoise &GetTerrainNoise() const { return m_Noise; }

private:
    void InitSpawnArea(Vector3f playerPos);

    void QueueChunk(Vector2i chunkPos);
    void PollPendingChunks();
    void UploadReadyChunks();
    void UnloadChunks(int renderDistance, Vector2i playerChunkPos);

    void MarkChunkDirty(Vector2i chunkPos);
    void RebuildDirtyChunks();

    ChunkSnapshot CreateSnapshotWithNeighbors(Ref<Chunk> chunk);

    void UploadMesh(ChunkMeshData &meshData);

private:
    static constexpr int MAX_CHUNKS_PER_FRAME = 1;

    TerrainNoise m_Noise;

    World &m_World;

    std::unordered_map<Vector2i, Ref<Chunk>> m_Chunks;
    std::unordered_map<Vector2i, PendingChunk> m_PendingChunks;
    std::deque<Vector2i> m_ChunkLoadQueue;

    Vector2i m_LastPlayerChunkPos = {INT_MAX, INT_MAX};
    std::unordered_set<Vector2i> m_InQueue;
    std::vector<Ref<Chunk>> m_ChunksReadyToRegister;

    std::unordered_set<Vector2i> m_DirtyChunks;
    std::deque<ReadyChunk> m_ReadyQueue;

    Ref<TextureArray2D> m_ChunkTextureArray;

    BS::thread_pool<> m_ThreadPool;

    bool m_IsSpawnAreaReady = false;
    Vector2i m_SpawnChunkPos;
};
