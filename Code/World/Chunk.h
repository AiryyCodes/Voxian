#pragma once

#include "Graphics/Shader.h"
#include "Math/Vector.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 512
#define CHUNK_BASE_HEIGHT 64

struct MeshData
{
    std::vector<float> Vertices;
    std::vector<unsigned int> Indices;
};

struct BlockData
{
    std::vector<uint8_t> Blocks; // 0 = air, 1 = solid, etc.

    BlockData()
    {
        Blocks.resize(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH, 0);
    }

    inline size_t Index(int x, int y, int z) const
    {
        return x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y);
    }

    inline bool Get(int x, int y, int z) const
    {
        if (x < 0 || x >= CHUNK_WIDTH ||
            y < 0 || y >= CHUNK_HEIGHT ||
            z < 0 || z >= CHUNK_WIDTH)
            return false; // outside chunk
        return Blocks[Index(x, y, z)] != 0;
    }

    inline void Set(int x, int y, int z, bool value)
    {
        if (x < 0 || x >= CHUNK_WIDTH ||
            y < 0 || y >= CHUNK_HEIGHT ||
            z < 0 || z >= CHUNK_WIDTH)
            return;
        Blocks[Index(x, y, z)] = value ? 1 : 0;
    }
};

class ChunkManager;
class Chunk
{
public:
    Chunk(ChunkManager *chunkManager, int x = 0, int z = 0) : m_ChunkManager(chunkManager), m_Position(Vector2i(x, z)) {}
    ~Chunk();

    void UploadMeshToGPU();
    void DeleteGPUData();
    void Draw(const Shader &shader);

    void SetBlockData(const BlockData &data);
    void SetMeshData(MeshData &data);

    bool GetBlock(int x, int y, int z) const
    {
        return m_Blocks.Get(x, y, z);
    }

private:
    friend class ChunkManager;

    ChunkManager *m_ChunkManager = nullptr;

    // std::vector<uint8_t> m_Blocks = std::vector<uint8_t>(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH, 0);
    BlockData m_Blocks;
    std::mutex m_Mutex;

    std::atomic<bool> m_NeedsRebuild{false};
    std::atomic<bool> m_BlocksReady{false};
    std::atomic<bool> m_MeshReady{false};
    std::atomic<bool> m_GpuReady{false};
    std::atomic<bool> m_IsGeneratingMesh{false};
    std::atomic<bool> m_ShouldUnload{false};

    std::atomic<int> m_PendingTasks{0};

    MeshData m_Mesh;
    std::mutex m_MeshMutex;

    unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;

    Vector2i m_Position;
};
