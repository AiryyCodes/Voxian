#include "Chunk.h"
#include "Component/Chunk/ChunkGenerator.h"
#include "Component/Chunk/ChunkMeshGenerator.h"
#include "Math/Vector.h"
#include "Renderer/ShaderLibrary.h"
#include "World/Entity/Component/MeshRenderer.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"

Chunk::Chunk(Vector2i position)
    : Entity("Chunk" + std::to_string(position.x) + "," + std::to_string(position.y)),
      m_Position(position)
{
    Transform &transform = AddComponent<Transform>();
    transform.Position = Vector3f(position.x * CHUNK_SIZE, 0.0f, position.y * CHUNK_SIZE);

    ChunkGenerator &generator = AddComponent<ChunkGenerator>();
    generator.Generate();

    ChunkMeshGenerator &meshGenerator = AddComponent<ChunkMeshGenerator>(*this);
    ChunkMeshData meshData = meshGenerator.GenerateMesh();

    MeshRenderer &meshRenderer = AddComponent<MeshRenderer>(Shaders::Chunk);
    meshRenderer.GetMesh().Init(meshData.Vertices.data(), meshData.Vertices.size() * sizeof(ChunkVertex), meshData.Indices.data(), meshData.Indices.size(), {
                                                                                                                                                                {AttribType::Float3, false}, // Position
                                                                                                                                                                {AttribType::Float3, false}  // Normal
                                                                                                                                                            });
}

Chunk::~Chunk()
{
}

bool Chunk::GetBlock(int x, int y, int z) const
{
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return false; // Out of bounds, treat as air

    return m_Blocks[x][y][z];
}

void Chunk::SetBlock(int x, int y, int z, bool solid)
{
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return; // Out of bounds, ignore

    m_Blocks[x][y][z] = solid;
}

bool Chunk::IsBlockSolid(int x, int y, int z) const
{
    return GetBlock(x, y, z);
}