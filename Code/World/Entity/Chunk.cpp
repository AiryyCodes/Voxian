#include "Chunk.h"
#include "Component/Chunk/ChunkGenerator.h"
#include "Component/Chunk/ChunkMeshGenerator.h"
#include "Engine.h"
#include "Math/Vector.h"
#include "Renderer/ShaderLibrary.h"
#include "Renderer/Texture.h"
#include "Util/Memory.h"
#include "World/Block/Block.h"
#include "World/Block/BlockRegistry.h"
#include "World/Chunk/ChunkManager.h"
#include "World/Entity/Component/MeshRenderer.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"
#include "World/World.h"

#include <stb/stb_image.h>

Chunk::Chunk(Vector2i position)
    : Entity("Chunk" + std::to_string(position.x) + "," + std::to_string(position.y)),
      m_Position(position)
{
    Transform &transform = AddComponent<Transform>();
    transform.Position = Vector3f(position.x * CHUNK_SIZE, 0.0f, position.y * CHUNK_SIZE);

    ChunkManager &chunkManager = EngineContext::GetWorld().GetChunkManager();

    AddComponent<ChunkGenerator>();
    AddComponent<ChunkMeshGenerator>(*this);
}

Chunk::~Chunk()
{
}

ChunkSnapshot Chunk::CreateSnapshot() const
{
    ChunkSnapshot snapshot;

    size_t totalSize = PADDED_CHUNK_SIZE * PADDED_CHUNK_HEIGHT * PADDED_CHUNK_SIZE;
    snapshot.Blocks.resize(totalSize);

    // Copy block data
    for (int x = 0; x < PADDED_CHUNK_SIZE; x++)
        for (int z = 0; z < PADDED_CHUNK_SIZE; z++)
            for (int y = 0; y < PADDED_CHUNK_HEIGHT; y++)
                snapshot.Blocks[x + PADDED_CHUNK_SIZE * (y + PADDED_CHUNK_HEIGHT * z)] = m_Blocks.GetId(x, y, z);

    // Snapshot block registry — no lock needed on worker thread after this
    BlockRegistry &registry = EngineContext::GetBlockRegistry();
    int blockCount = registry.GetNumBlocks();
    snapshot.BlockProperties.resize(blockCount);
    for (int i = 0; i < blockCount; i++)
    {
        const Block *block = registry.GetBlockByIndex(i);
        snapshot.BlockProperties[i] = block->GetProperties();
    }

    return snapshot;
}

void Chunk::UploadMesh(ChunkMeshData &meshData, Ref<TextureArray2D> texture)
{
    MeshRenderer &meshRenderer = HasComponent<MeshRenderer>()
                                     ? GetComponent<MeshRenderer>()
                                     : AddComponent<MeshRenderer>(Shaders::Chunk);
    meshRenderer.GetMesh().Init(
        meshData.Vertices.data(),
        meshData.Vertices.size() * sizeof(ChunkVertex),
        meshData.Indices.data(),
        meshData.Indices.size(),
        {
            {AttribType::UInt, false},   // Data1
            {AttribType::UInt, false},   // Data2
            {AttribType::Float4, false}, // UVBounds
        });

    meshRenderer.GetMesh().SetTexture(texture);
}

uint16_t Chunk::GetBlock(int x, int y, int z) const
{
    if (x < 0 || x >= CHUNK_SIZE ||
        y < 0 || y >= CHUNK_HEIGHT ||
        z < 0 || z >= CHUNK_SIZE)
        return 0;
    return m_Blocks.GetId(x + 1, y + 1, z + 1);
}

void Chunk::SetBlock(int x, int y, int z, uint16_t id)
{
    if (x < 0 || x >= CHUNK_SIZE ||
        y < 0 || y >= CHUNK_HEIGHT ||
        z < 0 || z >= CHUNK_SIZE)
        return;
    m_Blocks.SetId(x + 1, y + 1, z + 1, id);
}

int Chunk::GetTopBlockY(int x, int z) const
{
    // +1 for padding
    int paddedX = x + 1;
    int paddedZ = z + 1;

    // Start from the very top and move down
    for (int y = PADDED_CHUNK_HEIGHT - 1; y >= 0; y--)
    {
        uint16_t blockId = GetBlock(paddedX, y, paddedZ);

        const Block *block = EngineContext::GetBlockRegistry().GetBlockByIndex(blockId);
        if (block && !block->GetProperties().IsAir)
        {
            return y;
        }
    }

    return 0; // Entire column is air
}
