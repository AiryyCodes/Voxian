#pragma once

#include "Graphics/Shader.h"
#include "Math/Vector.h"
#include "Queue.h"
#include "ThreadPool.h"
#include "World/Chunk.h"

#include <memory>
#include <mutex>
#include <unordered_map>

class ChunkManager
{
public:
    std::unordered_map<Vector2i, std::shared_ptr<Chunk>> m_Chunks;
    std::mutex m_ChunkMutex;

    ThreadPool m_ThreadPool;

    int viewDistance = 16;

    void RequestChunk(int cx, int cz);
    std::shared_ptr<Chunk> GetChunk(int cx, int cz);

    void UpdatePlayerPosition(const Vector3 &pos);
    void Update(const Shader &shader);

    std::shared_ptr<Chunk> getChunk(int x, int z);

private:
    bool blockAtSafe(const BlockData &data, int x, int y, int z) const;
    bool isBlockSolidGlobal(int x, int y, int z, int chunkX, int chunkZ, const BlockData &localData);

    BlockData generateBlocks(int chunkX, int chunkZ);
    MeshData generateMesh(const BlockData &blockData, int chunkX, int chunkZ);

    bool IsSolid(const BlockData &data, int x, int y, int z) const;

    std::array<float, 4> GetVertexAOs(const BlockData &localData,
                                      const Vector3i &blockPos,
                                      const Vector3i &faceNormal,
                                      int chunkX, int chunkZ);
    std::array<Vector3i, 3> GetAONeighbors(int vertexIndex, const Vector3i &face);

    int getBlockIndex(int x, int y, int z) const;

private:
    Vector3f m_PlayerPosition;

    ThreadSafeQueue<ThreadTask<BlockData>> blockQueue;
    ThreadSafeQueue<ThreadTask<MeshData>> meshQueue;
};
