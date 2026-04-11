#pragma once

#include "Math/Vector.h"
#include "Renderer/Texture.h"
#include "Util/Memory.h"
#include "World/Chunk/ChunkMesh.h"
#include "World/Chunk/ChunkSnapshot.h"
#include "World/Entity/Component/Chunk/ChunkGenerator.h"
#include "World/Entity/Entity.h"

#include <cstdint>
#include <vector>

struct ChunkBlockData
{
    std::vector<uint16_t> Indices;

    ChunkBlockData()
    {
        Indices.resize(PADDED_CHUNK_SIZE * PADDED_CHUNK_HEIGHT * PADDED_CHUNK_SIZE);
    }

    uint32_t GetIndex(int x, int y, int z) const
    {
        return x + PADDED_CHUNK_SIZE * (z + PADDED_CHUNK_SIZE * y);
    }

    uint16_t GetId(int x, int y, int z) const
    {
        if (x < 0 || x >= PADDED_CHUNK_SIZE ||
            y < 0 || y >= PADDED_CHUNK_HEIGHT ||
            z < 0 || z >= PADDED_CHUNK_SIZE)
            return 0;
        return Indices[GetIndex(x, y, z)];
    }

    void SetId(int x, int y, int z, uint16_t id)
    {
        uint32_t index = GetIndex(x, y, z);
        if (index < Indices.size())
            Indices[index] = id;
    }
};

class Chunk : public Entity
{
public:
    Chunk(Vector2i position);
    ~Chunk();

    ChunkSnapshot CreateSnapshot() const;
    void UploadMesh(ChunkMeshData &meshData, Ref<TextureArray2D> texture);

    uint16_t GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, uint16_t id);

    Vector2i GetPosition() const { return m_Position; }
    Vector3i GetWorldPosition() const { return Vector3i(m_Position.x * CHUNK_SIZE, 0, m_Position.y * CHUNK_SIZE); }

private:
    friend class ChunkGenerator;

    Vector2i m_Position;

    ChunkBlockData m_Blocks;
};
