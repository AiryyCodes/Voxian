#include "Chunk.h"
#include "Component/Chunk/ChunkMeshGenerator.h"
#include "Math/Vector.h"
#include "Renderer/ShaderLibrary.h"
#include "World/Entity/Component/MeshRenderer.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"

Chunk::Chunk(Vector2i position)
    : Entity("Chunk" + std::to_string(position.x) + "," + std::to_string(position.y))
{
    Transform &transform = AddComponent<Transform>();
    transform.Position = Vector3f(position.x * CHUNK_SIZE, 0.0f, position.y * CHUNK_SIZE);

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