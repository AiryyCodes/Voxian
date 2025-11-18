#pragma once

#include "Graphics/Shader.h"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 512
#define CHUNK_BASE_HEIGHT 64

struct MeshData
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

struct BlockData
{
    std::vector<bool> blocks = std::vector<bool>(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH, false);
};

struct ChunkMeshContext
{
    BlockData &center;       // The chunk being meshed
    BlockData *neighbors[6]; // +X, -X, +Y, -Y, +Z, -Z neighbors
    // null pointers = not loaded yet
};

class ChunkManager;
class Chunk : public std::enable_shared_from_this<Chunk>
{
public:
    ChunkManager *chunkManager = nullptr;

    // Flat 3D array of blocks
    std::vector<bool> blocks = std::vector<bool>(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH, false);

    std::atomic<bool> needsRebuild{false};
    std::atomic<bool> blocksReady{false};
    std::atomic<bool> meshReady{false};
    std::atomic<bool> gpuReady{false};
    std::atomic<bool> isGeneratingMesh{false};

    MeshData mesh;
    std::mutex meshMutex;

    unsigned int vao = 0, vbo = 0, ebo = 0;

    int chunkX = 0, chunkZ = 0; // chunk world coordinates

    Chunk(ChunkManager *chunkManager, int x = 0, int z = 0) : chunkManager(chunkManager), chunkX(x), chunkZ(z) {}
    ~Chunk();

    bool getBlock(int x, int y, int z) const { return blocks[x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y)]; }
    void setBlock(int x, int y, int z, bool value) { blocks[x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y)] = value; }

    void generateBlocks();           // CPU-heavy: fill bool array
    void generateMesh();             // CPU-heavy: create vertices & indices
    void uploadMeshToGPU();          // OpenGL: create VAO/VBO/EBO
    void draw(const Shader &shader); // render
                                     //
    bool CanGenerateMesh() const;
    void TryGenerateMesh();

    void applyBlockData(BlockData &data);
    void applyMeshData(MeshData &data);

    std::array<float, 4> GetVertexAOs(const Vector3i &blockPos, const Vector3i &faceNormal);

private:
    bool IsSolid(int x, int y, int z) const;

    std::array<Vector3i, 3> GetAONeighbors(int vertexIndex, const Vector3i &face);
};
