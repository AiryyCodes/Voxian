#pragma once

#include "Graphics/Shader.h"
#include "Math/Vector.h"
#include "Memory.h"
#include "World/Block.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 256
#define CHUNK_BASE_HEIGHT 64

struct MeshData
{
    std::vector<float> Vertices;
    std::vector<unsigned int> Indices;
};

struct BlockData
{
    std::vector<uint16_t> Indices;

    BlockData()
    {
        Indices.resize(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH, 0); // 0 = air
    }

    uint16_t Index(int x, int y, int z) const { return x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y); }

    std::shared_ptr<BlockState> Get(int x, int y, int z) const
    {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_WIDTH)
            return nullptr;

        switch (Indices[Index(x, y, z)])
        {
        case 0:
            return BLOCK_AIR;
        case 1:
            return BLOCK_STONE;
        case 2:
            return BLOCK_DIRT;
        // ...
        default:
            return nullptr;
        }
    }

    void Set(int x, int y, int z, Ref<BlockState> block)
    {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_WIDTH)
            return;

        if (block == BLOCK_AIR)
            Indices[Index(x, y, z)] = 0;
        else if (block == BLOCK_STONE)
            Indices[Index(x, y, z)] = 1;
        else if (block == BLOCK_DIRT)
            Indices[Index(x, y, z)] = 2;
        // ...
    }

    bool IsAir(int x, int y, int z) const { return Get(x, y, z) == BLOCK_AIR; }
};

class ChunkManager;
class Chunk
{
public:
    enum class State : uint8_t
    {
        Empty,       // No data generated yet
        Generating,  // Blocks are being generated
        BlocksReady, // Blocks ready, mesh not yet generated
        MeshReady,   // Mesh generated but not uploaded to GPU
        Done         // Fully ready (mesh uploaded)
    };

    Chunk(ChunkManager *chunkManager, int x = 0, int z = 0)
        : m_ChunkManager(chunkManager), m_Position(x, z), m_State(State::Empty) {}

    ~Chunk();

    void UploadMeshToGPU();
    void DeleteGPUData();
    void Draw(const Shader &shader);

    void SetBlockData(const BlockData &data);
    void SetMeshData(MeshData &data);

    Ref<BlockState> GetBlock(int x, int y, int z) const
    {
        return m_Blocks.Get(x, y, z);
    }

    Vector2i GetPosition() const { return m_Position; }

    // Thread-safe state check
    State GetState() const { return m_State.load(std::memory_order_acquire); }
    void SetState(State state) { m_State.store(state, std::memory_order_release); }

    // Dirty/rebuild flag
    std::atomic<bool> m_NeedsRebuild{false};
    std::atomic<bool> m_ShouldUnload{false};

private:
    friend class ChunkManager;

    ChunkManager *m_ChunkManager = nullptr;

    BlockData m_Blocks;
    MeshData m_Mesh;

    mutable std::mutex m_Mutex; // Protects block & mesh data
    std::mutex m_MeshMutex;

    // GPU handles
    unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;

    Vector2i m_Position;

    // Current chunk state
    std::atomic<State> m_State;
};
