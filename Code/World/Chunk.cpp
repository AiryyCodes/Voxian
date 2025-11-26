
#include "World/Chunk.h"
#include "Logger.h"
#include "World/ChunkManager.h"
#include "Graphics/Texture.h"
#include "Graphics/Vertex.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Memory.h"
#include "World/Block.h"

#include <cstddef>
#include <glad/gl.h>
#include <FastNoiseLite.h>
#include <mutex>

Chunk::Chunk(ChunkManager *chunkManager, int x, int z)
    : m_ChunkManager(chunkManager), m_Position(x, z), m_State(State::Empty)
{
}

Chunk::~Chunk()
{
    if (m_State == State::Done)
        DeleteGPUData();
}

void Chunk::UploadMeshToGPU()
{
    if (m_State != State::MeshReady)
        return;

    std::scoped_lock lock(m_MeshMutex);

    auto uploadSection = [&](const MeshSection &src,
                             GLuint &vao, GLuint &vbo, GLuint &ebo)
    {
        if (src.Vertices.empty() || src.Indices.empty())
            return;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, src.Vertices.size() * sizeof(BlockVertex),
                     src.Vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, src.Indices.size() * sizeof(unsigned int),
                     src.Indices.data(), GL_STATIC_DRAW);

        GLsizei stride = sizeof(BlockVertex);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                              (void *)offsetof(BlockVertex, Position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                              (void *)offsetof(BlockVertex, Normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                              (void *)offsetof(BlockVertex, UV));
        glEnableVertexAttribArray(2);

        glVertexAttribIPointer(3, 2, GL_INT, stride,
                               (void *)offsetof(BlockVertex, TextureSize));
        glEnableVertexAttribArray(3);

        glVertexAttribIPointer(4, 1, GL_INT, stride,
                               (void *)offsetof(BlockVertex, Layer));
        glEnableVertexAttribArray(4);

        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride,
                              (void *)offsetof(BlockVertex, AO));
        glEnableVertexAttribArray(5);

        glBindVertexArray(0);
    };

    // Upload opaque mesh
    uploadSection(m_Mesh.Opaque, m_OpaqueVAO, m_OpaqueVBO, m_OpaqueEBO);

    // Upload transparent mesh
    uploadSection(m_Mesh.Transparent, m_TransVAO, m_TransVBO, m_TransEBO);

    // Done
    SetState(State::Done);
}

void Chunk::DeleteGPUData()
{
    std::scoped_lock lock(m_MeshMutex);

    if (m_OpaqueVAO)
    {
        glDeleteVertexArrays(1, &m_OpaqueVAO);
        m_OpaqueVAO = 0;
    }
    if (m_OpaqueVBO)
    {
        glDeleteBuffers(1, &m_OpaqueVBO);
        m_OpaqueVBO = 0;
    }
    if (m_OpaqueEBO)
    {
        glDeleteBuffers(1, &m_OpaqueEBO);
        m_OpaqueEBO = 0;
    }

    if (m_TransVAO)
    {
        glDeleteVertexArrays(1, &m_TransVAO);
        m_TransVAO = 0;
    }
    if (m_TransVBO)
    {
        glDeleteBuffers(1, &m_TransVBO);
        m_TransVBO = 0;
    }
    if (m_TransEBO)
    {
        glDeleteBuffers(1, &m_TransEBO);
        m_TransEBO = 0;
    }

    // If the mesh data still exists in RAM, revert to MeshReady state
    if (m_State == State::Done)
        SetState(State::MeshReady);
}

void Chunk::Draw(const Shader &shader)
{
    Vector3f chunkWorldPos{
        m_Position.x * CHUNK_WIDTH,
        0.0f,
        m_Position.y * CHUNK_WIDTH};

    Matrix4 model = glm::translate(Matrix4(1.0f), chunkWorldPos);
    shader.SetUniform("u_Model", model);

    Ref<TextureArray2D> texture = g_BlockRegistry.GetTexture();
    if (!texture)
        return;

    texture->Bind(shader);

    shader.SetUniform("u_Block.Diffuse", 0);
    shader.SetUniform("u_Block.MaxTexSize",
                      Vector2(texture->GetMaxWidth(), texture->GetMaxHeight()));
}

void Chunk::DrawOpaque(const Shader &shader)
{
    if (m_State != State::Done)
        return;

    Draw(shader);

    if (m_OpaqueVAO != 0 && m_Mesh.Opaque.Indices.size() > 0)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glBindVertexArray(m_OpaqueVAO);
        glDrawElements(GL_TRIANGLES, m_Mesh.Opaque.Indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Chunk::DrawTransparent(const Shader &shader)
{
    if (m_State != State::Done)
        return;

    Draw(shader);

    if (m_TransVAO != 0 && m_Mesh.Transparent.Indices.size() > 0)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glBindVertexArray(m_TransVAO);
        glDrawElements(GL_TRIANGLES, m_Mesh.Transparent.Indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Chunk::SetBlockData(const BlockData &data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Blocks = data;
    m_NeedsRebuild = true;
    SetState(State::BlocksReady);
}

void Chunk::SetMeshData(MeshData &data)
{
    std::lock_guard<std::mutex> lock(m_MeshMutex);
    m_Mesh.Opaque = data.Opaque;
    m_Mesh.Transparent = data.Transparent;
    SetState(State::MeshReady);
}

void Chunk::MarkMeshDirty()
{
    // Set the dirty flag
    m_NeedsRebuild.store(true, std::memory_order_release);

    // Optionally, if chunk is already meshed, mark it as ready to regenerate
    State currentState = GetState();
    if (currentState == State::MeshReady || currentState == State::Done)
    {
        SetState(State::BlocksReady);
    }
}

AABB Chunk::GetAABB() const
{
    glm::vec3 base = glm::vec3(
        m_Position.x * CHUNK_WIDTH,
        0,
        m_Position.y * CHUNK_WIDTH);

    return {
        base,
        base + glm::vec3(CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_WIDTH)};
}
