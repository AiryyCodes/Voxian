#include "Chunk.h"
#include "Component/Chunk/ChunkGenerator.h"
#include "Component/Chunk/ChunkMeshGenerator.h"
#include "Logger.h"
#include "Math/Vector.h"
#include "Renderer/ShaderLibrary.h"
#include "Renderer/Texture.h"
#include "World/Entity/Component/MeshRenderer.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"

#include <stb/stb_image.h>

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
                                                                                                                                                                {AttribType::Float3, false}, // Normal
                                                                                                                                                                {AttribType::Float2, false}  // UV
                                                                                                                                                            });
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *data = stbi_load("Assets/Textures/Block/grass_block.png", &width, &height, &channels, 0);

    LOG_INFO("Loaded texture with width: {}, height: {}, channels: {}", width, height, channels);

    int format = (channels == 4) ? GL_RGBA : GL_RGB;

    meshRenderer.GetMesh().SetTexture(width, height, data, format);

    stbi_image_free(data);
}

Chunk::~Chunk()
{
}

uint16_t Chunk::GetBlock(int x, int y, int z) const
{
    // Bounds check against CHUNK_SIZE, not PADDED_SIZE
    if (x < 0 || x >= CHUNK_SIZE ||
        y < 0 || y >= CHUNK_HEIGHT ||
        z < 0 || z >= CHUNK_SIZE)
        return 0;
    return m_Blocks.GetId(x + 1, y + 1, z + 1); // +1 offset into padded array
}

void Chunk::SetBlock(int x, int y, int z, uint16_t id)
{
    if (x < 0 || x >= CHUNK_SIZE ||
        y < 0 || y >= CHUNK_HEIGHT ||
        z < 0 || z >= CHUNK_SIZE)
        return;
    m_Blocks.SetId(x + 1, y + 1, z + 1, id);
}