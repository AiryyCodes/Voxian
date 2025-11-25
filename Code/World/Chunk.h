#pragma once

#include "Graphics/Shader.h"
#include "Graphics/Vertex.h"
#include "Math/AABB.h"
#include "Math/Vector.h"
#include "World/Block.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 256
#define CHUNK_BASE_HEIGHT 64

struct MeshSection
{
    std::vector<BlockVertex> Vertices;
    std::vector<unsigned int> Indices;
};

struct MeshData
{
    MeshSection Opaque;
    MeshSection Transparent;
};

struct BlockData
{
    static constexpr int PW = CHUNK_WIDTH + 2;  // padded width
    static constexpr int PH = CHUNK_HEIGHT + 2; // padded height

    std::vector<uint16_t> Indices;

    BlockData()
    {
        Indices.resize(PW * PH * PW, 0);
    }

    inline uint32_t Index(int x, int y, int z) const
    {
        return x + PW * (z + PW * y);
    }

    inline uint16_t GetID(int x, int y, int z) const
    {
        return Indices[Index(x, y, z)];
    }

    inline void SetID(int x, int y, int z, uint16_t id)
    {
        Indices[Index(x, y, z)] = id;
    }

    inline const BlockState &Get(int x, int y, int z) const
    {
        return g_BlockRegistry.Get(GetID(x, y, z));
    }

    inline bool IsAir(int x, int y, int z) const
    {
        return GetID(x, y, z) == 0;
    }
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

    Chunk(ChunkManager *chunkManager, int x, int z);

    ~Chunk();

    void UploadMeshToGPU();
    void DeleteGPUData();

    void Draw(const Shader &shader);
    void DrawOpaque(const Shader &shader);
    void DrawTransparent(const Shader &shader);

    void SetBlockData(const BlockData &data);
    void SetMeshData(MeshData &data);

    const BlockState &GetBlock(int x, int y, int z) const
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

    void MarkMeshDirty();

    AABB GetAABB() const;

private:
    friend class ChunkManager;

    ChunkManager *m_ChunkManager = nullptr;

    BlockData m_Blocks;
    MeshData m_Mesh;

    mutable std::mutex m_Mutex;
    std::mutex m_MeshMutex;

    unsigned int m_OpaqueVAO = 0, m_OpaqueVBO = 0, m_OpaqueEBO = 0;
    unsigned int m_TransVAO = 0, m_TransVBO = 0, m_TransEBO = 0;

    Vector2i m_Position;

    std::atomic<State> m_State;
};
