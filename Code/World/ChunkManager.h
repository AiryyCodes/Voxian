#pragma once

#include "FastNoiseLite.h"
#include "Graphics/Shader.h"
#include "Math/Noise.h"
#include "Math/Vector.h"
#include "Queue.h"
#include "ThreadPool.h"
#include "World/Block.h"
#include "World/Chunk.h"
#include "World/Structure.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class ChunkManager
{
public:
    ChunkManager();

    void RequestChunk(int x, int z);
    std::shared_ptr<Chunk> GetChunk(int x, int z);

    void UpdatePlayerPosition(const Vector3 &pos);
    void Update(const Shader &shader);

private:
    const BlockState &GetBlock(int x, int y, int z, int chunkX, int chunkZ, const BlockData &localData);

    int GetBlockIndex(int x, int y, int z) const;

    BlockData GenerateBlocks(int chunkX, int chunkZ);
    MeshData GenerateMesh(const BlockData &blockData, int chunkX, int chunkZ);

    std::array<float, 4> GetVertexAOs(const BlockData &localData, const Vector3i &blockPos, const Vector3i &faceNormal, const Vector2i chunkPos);
    std::array<Vector3i, 3> GetAONeighbors(int vertexIndex, const Vector3i &face);

    std::vector<TreeBlock> GenerateTreeBlocks(const Vector3i &base);
    bool TreeIntersectsChunk(const Tree &tree, const Vector2i &chunkPos) const;
    void PlaceTree(const Tree &tree, BlockData &data, const Vector2i &chunkPos);

    void GenerateTreesForChunk(int cx, int cz);
    void ApplyStructures(int cx, int cz, BlockData &data);

    bool CanPlaceTree(const Vector3i &pos, int chunkX, int chunkZ);

private:
    const int m_MaxRebuildsPerFrame = 2;

    int m_ViewDistance = 12;

    Vector3f m_PlayerPosition;

    ThreadSafeQueue<ThreadTask<BlockData>> m_BlockQueue;
    ThreadSafeQueue<ThreadTask<MeshData>> m_MeshQueue;

    std::unordered_map<Vector2i, std::shared_ptr<Chunk>> m_Chunks;
    std::unordered_set<Vector2i> m_RequestedChunks;
    std::recursive_mutex m_ChunkMutex;

    Shader shadowShader;
    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;

    ThreadPool m_ThreadPool;

    Noise m_Noise;

    std::mutex m_TreesMutex;
    std::unordered_map<Vector2i, std::vector<Tree>> m_Trees;
};
