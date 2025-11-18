#pragma once

#include "Graphics/Shader.h"
#include "Math/Vector.h"

#include <atomic>
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
    std::vector<bool> Blocks = std::vector<bool>(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH, false);
};

class ChunkManager;
class Chunk
{
public:
    Chunk(ChunkManager *chunkManager, int x = 0, int z = 0) : m_ChunkManager(chunkManager), m_Position(Vector2i(x, z)) {}
    ~Chunk();

    void UploadMeshToGPU();
    void Draw(const Shader &shader);

    void SetBlockData(BlockData &data);
    void SetMeshData(MeshData &data);

private:
    friend class ChunkManager;

    ChunkManager *m_ChunkManager = nullptr;

    std::vector<bool> m_Blocks = std::vector<bool>(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH, false);

    std::atomic<bool> m_NeedsRebuild{false};
    std::atomic<bool> m_BlocksReady{false};
    std::atomic<bool> m_MeshReady{false};
    std::atomic<bool> m_GpuReady{false};
    std::atomic<bool> m_IsGeneratingMesh{false};

    MeshData m_Mesh;
    std::mutex m_MeshMutex;

    unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;

    Vector2i m_Position;
};
